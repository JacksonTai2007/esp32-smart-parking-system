# tests — 测试说明

本项目是真实硬件项目，MVP 阶段的验证分三层：

| 层级 | 在哪里跑 | 验证什么 |
| --- | --- | --- |
| 编译检查 | GitHub Actions（`.github/workflows/arduino-compile.yml`）/ `scripts/compile.sh` | 固件能否编译通过 |
| 硬件手动测试 | 用户本地（真实 ESP32 + 传感器） | 各模块真实行为、去抖、电平逻辑、计费正确性 |
| 单元测试（规划中） | 主机端，Phase 2 | 计费纯逻辑（`BillingService`） |

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
| 1 | 编译通过 | 本地或 CI 跑 `scripts/compile.sh` | 退出码 0；CI 绿勾 |
| 2 | 启动自检 | 上电 | 打印 banner `ESP32 Smart Parking System (slots + billing)`，各模块 `init OK`（Alert/OLED/Slots/Bill/PM）；OLED 显示 Smart Parking |
| 3 | OLED 显示 | 全程观察 | 显示 `Free x/总数`、车位地图（`1:_ 2:X ...`）、累计收入与次数（`Total 0.00 n0`）、最近事件消息；不亮则确认 I2C 地址 0x3C/0x3D 与接线 |
| 4 | 车位传感器 | 逐个遮挡红外 | `[Slots] slot N -> OCCUPIED (free x/4)`；移开后 `-> FREE`；若反了改 `SLOT_OCCUPIED_LEVEL` |
| 5 | 车辆入场 | 遮挡某车位（占用） | 识别入场、开始计时，短响 **1 次**，消息 `PN in - timing started`，OLED/Web 该位变占用并显示实时停留时长 |
| 6 | 车辆离场计费 | 移开该车位（空闲） | `[Bill] slot N parked …ms -> fee … cents`，短响 **2 次**，OLED/Web 显示本次费用与累计收入，最近记录追加一条；停留 ≤ 免费时长时费用为 0 |
| 7 | 去抖有效 | 在传感器前快速抖动手 | 抖动不会误触发进出场（`SLOT_DEBOUNCE_MS` 内的跳变被吸收）；必要时调大该值 |
| 8 | 满位提示 | 遮挡全部 4 个车位 | 最后一位占用时长响 **1 次**，消息 `PN in - Parking FULL`，OLED/Web `Free 0/总数` |
| 9 | Web 仪表盘 | 浏览器/手机访问 | 显示总/占用/剩余车位、车位网格（占用显示实时停留时长）、累计收入/停车次数/单价、最近停车记录、系统消息与运行时间，每 1 秒刷新 |
| 10 | Web 清零 | 点"清零累计收入" | `POST /api/reset` 后累计收入与最近记录归零（`ENABLE_WEB_MANUAL_CONTROL=0` 时返回 403） |
| 11 | Web STA | 配好 `WifiCredentials.h` | 日志打印 IP，浏览器访问该 IP 看仪表盘 |
| 12 | Web AP 兜底 | 删除/不建凭据文件 | 日志打印 AP SSID，手机连 `ESP32-Parking` 访问 192.168.4.1 |
| 13 | 参数生效 | 改 `Settings.h` 单价/免费时长重新烧录 | `[Bill] init OK, free Ns then M cents/min` 随之变化，计费结果按新参数 |

## 3. JSON API 快速验证

```bash
curl http://<ESP32-IP>/api/status
curl -X POST http://<ESP32-IP>/api/reset
```

`/api/reset` 受 `ENABLE_WEB_MANUAL_CONTROL` 控制：开关为 1 时返回
`{"ok":true,...}` 并清零累计收入；为 0 时返回 403、仪表盘只读。

## 4. Phase 2 规划：主机端单元测试

`BillingService::computeFeeCents` 是纯函数（输入停留时长、输出费用「分」），
可直接在主机上做单元测试，覆盖免费时长边界、向上取整、单价换算等用例，无需 mock 硬件。
本期不强制，避免过度设计。
