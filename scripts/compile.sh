#!/usr/bin/env bash
# =====================================================================
# compile.sh — Arduino CLI 编译检查（云端 / 本地 / CI 通用）
#
# 用法:
#   ./scripts/compile.sh                 # 直接编译（要求环境已就绪）
#   ./scripts/compile.sh --install-deps  # 先安装 ESP32 core 和依赖库再编译
#
# 退出码约定（供 CI 和云端 Agent 区分失败原因）:
#   0  编译通过
#   1  编译失败 —— 代码问题
#   2  环境缺失 —— arduino-cli 或 ESP32 core 不存在，不代表代码有错
# =====================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FQBN="${FQBN:-esp32:esp32:esp32}"
SKETCH="${SKETCH:-$REPO_ROOT/firmware/parking-system}"
ESP32_BOARD_URL="${ESP32_BOARD_URL:-https://espressif.github.io/arduino-esp32/package_esp32_index.json}"
REQUIRED_LIBS=("ESP32Servo" "MFRC522" "Adafruit GFX Library" "Adafruit SSD1306")

INSTALL_DEPS=0
for arg in "$@"; do
    case "$arg" in
        --install-deps) INSTALL_DEPS=1 ;;
        *) echo "[compile.sh] unknown argument: $arg"; exit 2 ;;
    esac
done

if ! command -v arduino-cli >/dev/null 2>&1; then
    cat <<'EOF'
[compile.sh] ERROR: 未找到 arduino-cli —— 这是环境缺失，不是代码编译失败。

安装方法:
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
  export PATH="$PWD/bin:$PATH"
或参考: https://arduino.github.io/arduino-cli/latest/installation/

安装后运行: ./scripts/compile.sh --install-deps
EOF
    exit 2
fi

if [[ "$INSTALL_DEPS" == "1" ]]; then
    echo "[compile.sh] installing ESP32 core and libraries ..."
    arduino-cli core update-index --additional-urls "$ESP32_BOARD_URL"
    arduino-cli core install esp32:esp32 --additional-urls "$ESP32_BOARD_URL"
    arduino-cli lib update-index
    arduino-cli lib install "${REQUIRED_LIBS[@]}"
fi

if ! arduino-cli core list | grep -q "esp32:esp32"; then
    cat <<EOF
[compile.sh] ERROR: 未安装 ESP32 board core —— 这是环境缺失，不是代码编译失败。

安装方法:
  ./scripts/compile.sh --install-deps
或手动:
  arduino-cli core install esp32:esp32 --additional-urls $ESP32_BOARD_URL
EOF
    exit 2
fi

echo "[compile.sh] arduino-cli $(arduino-cli version | head -n1)"
echo "[compile.sh] compiling $SKETCH (FQBN: $FQBN) ..."

if arduino-cli compile --fqbn "$FQBN" --warnings default "$SKETCH"; then
    echo "[compile.sh] PASS: firmware compiles"
    exit 0
else
    echo "[compile.sh] FAIL: 编译失败（代码问题），请查看上方编译器输出"
    exit 1
fi
