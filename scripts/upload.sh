#!/usr/bin/env bash
# =====================================================================
# upload.sh — 本地烧录脚本（仅限连接了真实 ESP32 的本地电脑使用）
#
# !!! 云端 Agent（Claude Code Cloud / Codex Cloud）没有串口硬件，
# !!! 不能使用本脚本，也不得声称已完成真实烧录。
#
# 用法:
#   ./scripts/upload.sh                  # 默认串口 /dev/ttyUSB0
#   ./scripts/upload.sh /dev/ttyUSB0     # Linux
#   ./scripts/upload.sh /dev/cu.usbserial-0001   # macOS
#   ./scripts/upload.sh COM5             # Windows (Git Bash)
#
# 烧录后查看串口日志:
#   arduino-cli monitor -p <串口> -c baudrate=115200
# =====================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FQBN="${FQBN:-esp32:esp32:esp32}"
SKETCH="${SKETCH:-$REPO_ROOT/firmware/parking-system}"
PORT="${1:-/dev/ttyUSB0}"

if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "[upload.sh] ERROR: 未找到 arduino-cli，请先安装（见 scripts/compile.sh 提示）"
    exit 2
fi

if [[ "$PORT" == /dev/* && ! -e "$PORT" ]]; then
    echo "[upload.sh] WARNING: 串口 $PORT 不存在。"
    echo "  可用串口列表:  arduino-cli board list"
    echo "  （云端环境没有串口设备，此脚本只能在本地使用）"
    exit 2
fi

echo "[upload.sh] compiling before upload ..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH"

echo "[upload.sh] uploading to $PORT ..."
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SKETCH"

echo "[upload.sh] done. 查看串口日志:"
echo "  arduino-cli monitor -p $PORT -c baudrate=115200"
