# tests — 测试说明

本项目是真实硬件项目，MVP 阶段的验证分三层：

| 层级 | 在哪里跑 | 验证什么 |
| --- | --- | --- |
| 编译检查 | GitHub Actions（`.github/workflows/arduino-compile.yml`）/ `scripts/compile.sh` | 固件能否编译通过 |
| 硬件手动测试 | 用户本地（真实 ESP32 + 传感器） | 各模块真实行为、阈值、电平逻辑 |
| 单元测试（规划中） | 主机端，Phase 2 | 状态机纯逻辑 |

## 1. 编译检查（云端 / CI / 本地均可）

```bash
./scripts/compile.sh --install-deps   # 首次（安装 ESP32 core + 依赖库）
./scripts/compile.sh                  # 之后
```

退出码：`0` 编译通过；`1` 代码问题；`2` 环境缺失（不代表代码有错）。

## 2. 硬件手动测试清单（本地，烧录后逐项执行）

按下面顺序测试，每一步把串口日志（115200 波特率）保存下来贴回 PR：

| # | 测试项 | 操作 | 预期串口日志 / 现象 |
| --- | --- | --- | --- |
| 1 | 启动自检 | 上电 | 打印 banner、各模块 `init OK`；OLED 显示 Smart Parking |
| 2 | RC522 在线 | 上电 | `[RFID] init OK, RC522 firmware version 0x91/0x92`（0x00/0xFF 为接线错误） |
| 3 | 读卡 UID | 刷任意卡 | `[RFID] New card UID: XX:XX:...`，把 UID 加入 `Settings.h` 白名单 |
| 4 | 超声波 | 手掌靠近 <20cm | `[Sonar] vehicle detected at xx.x cm`；移开后 `vehicle left` |
| 5 | 车位传感器 | 逐个遮挡红外 | `[Slots] slot N -> OCCUPIED (free x/4)`；若反了改 `SLOT_OCCUPIED_LEVEL` |
| 6 | 完整入场流程 | 靠近→刷白名单卡 | 状态机日志 idle→…→card_accepted，短响 1 次，舵机 0°→90°，5 秒后自动回 0° |
| 7 | 无效卡 | 刷未登记卡 | card_rejected，短响 3 次，闸机不动 |
| 8 | 满位 | 遮挡全部 4 个车位再靠近 | parking_full，长响 1 次，不开闸，OLED 显示 Parking FULL |
| 9 | OLED | 全程观察 | 剩余车位/闸机状态/消息实时变化；不亮则确认 I2C 地址 0x3C/0x3D |
| 10 | Web STA | 配好 `WifiCredentials.h` | 日志打印 IP，浏览器访问该 IP 看仪表盘 |
| 11 | Web AP 兜底 | 删除/不建凭据文件 | 日志打印 AP SSID，手机连 `ESP32-Parking` 访问 192.168.4.1 |
| 12 | Web 手动控制 | 点开闸/关闸/重置 | 舵机响应，页面 1 秒内刷新状态 |

## 3. JSON API 快速验证

```bash
curl http://<ESP32-IP>/api/status
curl -X POST http://<ESP32-IP>/api/gate/open
curl -X POST http://<ESP32-IP>/api/gate/close
curl -X POST http://<ESP32-IP>/api/reset
```

## 4. Phase 2 规划：主机端单元测试

`ParkingStateMachine` 的纯逻辑（状态迁移、超时、满位分支）可以在主机上用
mock 的服务接口做单元测试。前置工作：把状态机对各服务的依赖抽象成接口。
本期不做，避免过度设计。
