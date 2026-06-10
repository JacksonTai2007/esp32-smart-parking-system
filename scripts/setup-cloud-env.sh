#!/usr/bin/env bash
# =====================================================================
# setup-cloud-env.sh — 在受限网络的云容器内搭建 Arduino 编译环境
#
# 适用场景：云端 Agent 容器（Claude Code Cloud / Codex Cloud）的网络
# 策略拦截了 Arduino 官方下载源，但放行 GitHub：
#   ✗ downloads.arduino.cc      （arduino-cli 官方源 / 库注册表 / 官方工具）
#   ✗ espressif.github.io       （ESP32 board 索引官方地址）
#   ✓ github.com / raw.githubusercontent.com / objects.githubusercontent.com
#
# 本脚本把所有组件改从 GitHub 获取（已在 Claude Code Cloud 实测可行）：
#   1. arduino-cli        <- github.com/arduino/arduino-cli releases
#   2. ESP32 board 索引   <- github.com/espressif/arduino-esp32 release 资产
#                            （官方索引的逐字节副本，按 core 版本号取）
#      补丁：剥离 arduino:dfu-util 工具依赖 —— 它只在 USB 烧录时使用，
#      托管在被拦截的官方源上；云端只编译不烧录，剥离无影响。
#   3. esp32 core         <- 平台与工具链均为 github.com release 资产
#   4. 四个依赖库（钉版本，与 CI 一致） <- git clone 安装
#   5. ctags              <- github.com/arduino/ctags releases
#                            （arduino-cli 预处理 .ino 必需，属 builtin 工具）
#
# 用法：
#   ./scripts/setup-cloud-env.sh
#   export PATH="$HOME/tools/arduino-cli:$PATH"
#   ./scripts/compile.sh
#
# 依赖：bash、curl、tar、git、python3（补丁索引用）
# 本地开发机网络不受限时不需要本脚本，直接 compile.sh --install-deps。
# =====================================================================
set -euo pipefail

CLI_VERSION="${CLI_VERSION:-1.5.1}"
ESP32_CORE_VERSION="${ESP32_CORE_VERSION:-3.3.10}"
CTAGS_VERSION="5.8-arduino11"
INSTALL_DIR="${INSTALL_DIR:-$HOME/tools/arduino-cli}"
DATA_DIR="$HOME/.arduino15"
IDX_DIR="$HOME/.arduino15-github-index"

# 与 .github/workflows/arduino-compile.yml 安装的库版本保持一致
LIB_GIT_SPECS=(
    "https://github.com/madhephaestus/ESP32Servo.git#3.2.1"
    "https://github.com/miguelbalboa/rfid.git#1.4.12"
    "https://github.com/adafruit/Adafruit-GFX-Library.git#1.12.6"
    "https://github.com/adafruit/Adafruit_SSD1306.git#2.5.17"
    "https://github.com/adafruit/Adafruit_BusIO.git#1.17.4"
)

log() { echo "[setup-cloud-env] $*"; }

# ---- 1. arduino-cli ----
if command -v arduino-cli >/dev/null 2>&1; then
    log "arduino-cli 已存在: $(command -v arduino-cli)"
else
    log "下载 arduino-cli ${CLI_VERSION} (GitHub releases) ..."
    mkdir -p "$INSTALL_DIR"
    curl -fsSL -o "$INSTALL_DIR/cli.tar.gz" \
        "https://github.com/arduino/arduino-cli/releases/download/v${CLI_VERSION}/arduino-cli_${CLI_VERSION}_Linux_64bit.tar.gz"
    tar xzf "$INSTALL_DIR/cli.tar.gz" -C "$INSTALL_DIR"
    rm -f "$INSTALL_DIR/cli.tar.gz"
    export PATH="$INSTALL_DIR:$PATH"
    log "arduino-cli 安装完成: $(arduino-cli version | head -n1)"
fi

# ---- 2. ESP32 board 索引（GitHub release 资产副本 + dfu-util 剥离补丁）----
mkdir -p "$IDX_DIR" "$DATA_DIR"
RAW_IDX="$IDX_DIR/raw_index.json"
PATCHED_IDX="$IDX_DIR/package_esp32_index.json"
if [[ ! -f "$PATCHED_IDX" ]]; then
    log "下载 ESP32 board 索引 (core ${ESP32_CORE_VERSION} release 资产) ..."
    curl -fsSL -o "$RAW_IDX" \
        "https://github.com/espressif/arduino-esp32/releases/download/${ESP32_CORE_VERSION}/package_esp32_index.json"
    log "剥离 ${ESP32_CORE_VERSION} 条目中非 esp32 包的工具依赖 (arduino:dfu-util, 仅烧录用) ..."
    TARGET_VERSION="$ESP32_CORE_VERSION" RAW="$RAW_IDX" OUT="$PATCHED_IDX" python3 - <<'PYEOF'
import json, os
with open(os.environ["RAW"]) as f:
    idx = json.load(f)
for pkg in idx["packages"]:
    for p in pkg["platforms"]:
        if p["version"] == os.environ["TARGET_VERSION"]:
            p["toolsDependencies"] = [
                t for t in p.get("toolsDependencies", []) if t.get("packager") == "esp32"
            ]
with open(os.environ["OUT"], "w") as f:
    json.dump(idx, f)
print("patched index ->", os.environ["OUT"])
PYEOF
fi
IDX_URL="file://$PATCHED_IDX"

# ---- 3. esp32 core ----
if arduino-cli core list 2>/dev/null | grep -q "esp32:esp32[[:space:]]*${ESP32_CORE_VERSION}"; then
    log "esp32 core ${ESP32_CORE_VERSION} 已安装"
else
    log "安装 esp32:esp32@${ESP32_CORE_VERSION} (下载量较大, 请耐心等待) ..."
    # 默认 Arduino 官方索引在受限网络必然 403, 容忍其失败
    arduino-cli core update-index --additional-urls "$IDX_URL" || true
    arduino-cli core install "esp32:esp32@${ESP32_CORE_VERSION}" --additional-urls "$IDX_URL"
fi

# ---- 4. 库（git 方式, 绕过被拦截的库注册表）----
export ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL=true
[[ -f "$DATA_DIR/library_index.json" ]] || echo '{"libraries":[]}' > "$DATA_DIR/library_index.json"
for spec in "${LIB_GIT_SPECS[@]}"; do
    name=$(basename "${spec%%#*}" .git)
    log "安装库: $spec"
    arduino-cli lib install --git-url "$spec" || log "WARNING: $name 安装失败, 请手动检查 tag 是否存在"
done

# ---- 5. ctags（arduino-cli 预处理 .ino 必需的 builtin 工具）----
CTAGS_DIR="$DATA_DIR/packages/builtin/tools/ctags/$CTAGS_VERSION"
if [[ -x "$CTAGS_DIR/ctags" ]]; then
    log "ctags 已就位"
else
    log "下载 ctags ${CTAGS_VERSION} (GitHub releases) ..."
    mkdir -p "$CTAGS_DIR"
    curl -fsSL -o /tmp/ctags.tar.bz2 \
        "https://github.com/arduino/ctags/releases/download/${CTAGS_VERSION}/ctags-${CTAGS_VERSION}-x86_64-pc-linux-gnu.tar.bz2"
    tar xjf /tmp/ctags.tar.bz2 -C "$CTAGS_DIR"   # 包内为裸 ctags 二进制
    chmod +x "$CTAGS_DIR/ctags"
fi

log "环境就绪。运行:"
log "  export PATH=\"$INSTALL_DIR:\$PATH\""
log "  ./scripts/compile.sh"
log "（启动时关于默认索引 403 / serial-discovery 缺失的报错可忽略, 不影响编译）"
