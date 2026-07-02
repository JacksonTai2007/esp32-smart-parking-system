# 基于 ESP32 的智能停车场管理系统设计与实现

ESP32 Smart Parking System — 大学毕业设计项目

[![Arduino Compile](https://github.com/JacksonTai2007/esp32-smart-parking-system/actions/workflows/arduino-compile.yml/badge.svg)](https://github.com/JacksonTai2007/esp32-smart-parking-system/actions/workflows/arduino-compile.yml)

## 1. 项目简介

以 ESP32 为主控的智能停车场管理系统：红外车位传感器实时识别每个车位有无车辆
（车辆识别），自动统计剩余车位（车位管理），并按每辆车的停留时长计费
（按分钟，本地计算与展示）。OLED 与本地 Web 页面同步展示车位状态、累计收入与
最近停车记录，蜂鸣器提供入场 / 离场 / 满位声音反馈。

核心演示流程：

```
车辆驶入车位 → 红外检测占用 → 道闸自动抬杆放行 → 识别入场并开始计时（蜂鸣 1 短响）
→ 车辆驶离 → 道闸自动抬杆 + 按停留时长结算费用（蜂鸣 2 短响）→ 数秒后自动落杆
→ OLED / Web 显示本次费用、累计收入与最近停车记录
```

> 说明：本期聚焦"车位识别 + 车位管理 + 按时长计费"的最小闭环，并加入 **SG90 出入口
> 道闸**（进出场自动抬杆、网页手动开 / 落，可由 `ENABLE_GATE` 关闭）。车辆放行**完全由
> 红外车辆识别驱动，不做 RFID 刷卡鉴权**；HC-SR04 入口超声波亦未纳入（见第 14 节）。

## 2. 毕设定位

- 课题：基于 ESP32 的智能停车场管理系统设计与实现
- 当前阶段：**Phase 1 MVP**（车位识别 + 计费的最小可演示闭环），
  论文大纲见 [docs/thesis-outline.md](docs/thesis-outline.md)
- 项目特色之一：采用**云端 AI Agent 协作开发流程**（见第 7 节），
  开发过程本身可作为论文的工程方法章节素材
- 答辩演示脚本见 [docs/demo-script.md](docs/demo-script.md)

## 3. 功能列表

### Phase 1（本期已实现，待硬件实测）
- ✅ 2~4 路红外车位检测：识别每个车位有无车辆（软件去抖、触发电平可配置）
- ✅ 车位事件驱动：车位占用 → 识别车辆入场并开始计时；车位空闲 → 车辆离场
- ✅ 按时长计费：免费时长内不计费，超出按分钟向上取整 × 单价（整数"分"运算）
- ✅ 营收统计：累计总收入、停车次数、最近若干条停车记录
- ✅ OLED SSD1306 实时显示车位地图、累计收入与最近事件
- ✅ 蜂鸣器提示（入场 1 短响 / 离场 2 短响 / 满位 1 长响）
- ✅ SG90 出入口道闸：进出场自动抬杆，保持数秒自动落杆；网页可手动开 / 落
  （非阻塞舵机驱动，可由 `ENABLE_GATE` 关闭）
- ✅ 本地 Web 仪表盘（停车场俯视平面图）+ JSON 状态 API + 一键清零收入（STA 失败自动开热点兜底）
- ✅ 演示 / 模拟模式（`ENABLE_SIM_MODE`）：无红外传感器也能用网页按钮模拟车辆进出，
  跑通完整计费闭环，便于录制演示视频（详见 [docs/model-build.md](docs/model-build.md)）

### Phase 2（规划，引脚已预留）
火焰/烟雾报警与风扇联动（GPIO 27/36/16）、对射式过车计数、
数据持久化与计费记录导出等。（不做 RFID 刷卡鉴权，放行只靠红外识别车辆。）

## 4. 硬件清单

> 下表是**本系统当前实际使用的器件子集**。手上的实物套件是**高配版**
> （33 个模块），完整实物清单与各模块"已用 / Phase 2 预留 / 备用候选"分类
> 见 [docs/hardware-inventory.md](docs/hardware-inventory.md)。

| 器件 | 数量 | 说明 |
| --- | --- | --- |
| ESP32 DevKit（WROOM-32） | 1 | 主控 |
| 红外避障传感器 | 2~4 | 车位占用检测（车辆识别） |
| OLED SSD1306 128×64（I2C） | 1 | 状态显示 |
| 有源蜂鸣器模块 | 1 | 声音提示 |
| SG90 舵机 | 1 | 出入口道闸（可由 `ENABLE_GATE` 关闭，不接也能运行） |
| 面包板、杜邦线、小车模型 | 若干 | 搭建与演示 |

整体功耗很低，USB 5V 可直接供电；单个 SG90 一般也能由开发板 5V 带动，若舵机
抖动或导致 ESP32 复位，改用独立 5V 供电并与开发板**共地**。

## 5. 接线表

完整接线说明与排查方法见 [docs/pinout.md](docs/pinout.md) 和
[docs/hardware-wiring.md](docs/hardware-wiring.md)，代码权威定义在
[`firmware/parking-system/config/Pins.h`](firmware/parking-system/config/Pins.h)。

| 功能 | 模块 | ESP32 引脚 | 备注 |
| --- | --- | --- | --- |
| I2C SDA / SCL | OLED SSD1306 | GPIO 21 / 22 | I2C |
| 蜂鸣器 | Buzzer | GPIO 14 | 数字输出 |
| 出入口道闸 | SG90 舵机 | GPIO 13 | PWM（LEDC 驱动） |
| 车位 1~4 | 红外传感器 | GPIO 32 / 33 / 34 / 35 | 34/35 输入专用 |
| 火焰 / 烟雾 / 风扇 | Phase 2 预留 | GPIO 27 / 36 / 16 | 本期不接 |

⚠️ 关键项：ESP32 与各模块必须**共地**；GPIO 34/35 为输入专用脚，
红外模块自带推挽输出可直接接入。

**默认硬件假设**（实测不符时只需改 `config/Settings.h`）：开发板为 ESP32 DevKit
（WROOM-32）；车位传感器为红外避障模块，**检测到物体输出 LOW**
（`SLOT_OCCUPIED_LEVEL = LOW`）；蜂鸣器为高电平触发的有源模块；车位数 4
（可配 2~4）；串口波特率 **115200**。

## 6. 软件架构

模块化固件 + 非阻塞 `millis()` 事件驱动，详见 [docs/architecture.md](docs/architecture.md)。

| 模块 | 职责 |
| --- | --- |
| `ParkingManager` | 主业务：识别车辆进出、触发计费、生成状态快照 |
| `SlotManager` | 红外车位检测 + 软件去抖 + 剩余车位统计 |
| `BillingService` | 按时长计费 + 营收/停车记录统计 |
| `DisplayService` | OLED 显示 |
| `AlertService` | 蜂鸣器节奏 |
| `GateService` | 出入口道闸（SG90 舵机，LEDC PWM，非阻塞抬杆/落杆） |
| `WebDashboard` | 网页 + JSON API + Wi-Fi 管理 |

约定：GPIO 只在 `config/Pins.h` 定义；阈值/单价/时长/功能开关只在
`config/Settings.h` 定义；主循环无长 `delay()`。

## 7. 云端 AI Agent 工作流

本项目采用云端 AI 协作开发，完整流程见 [docs/cloud-workflow.md](docs/cloud-workflow.md)：

- **Claude Code Cloud** — 主功能开发、架构、跨文件实现、文档（守则：[CLAUDE.md](CLAUDE.md)）
- **Codex Cloud** — PR 审查、编译验证、可维护性检查（守则：[AGENTS.md](AGENTS.md)）
- **GitHub PR** — 协作中心；GitHub Actions 自动编译检查
- **用户本地** — 真实硬件接线、烧录、串口日志、阈值调试、答辩演示

云端 Agent 无法烧录与读取串口，每个 PR 都会明确标注
"云端已验证内容"与"需本地硬件验证内容"。

## 8. 如何在云端 / CI 编译

```bash
./scripts/compile.sh --install-deps   # 首次：安装 ESP32 core + 依赖库
./scripts/compile.sh                  # 之后：仅编译
```

- FQBN：`esp32:esp32:esp32`；退出码 `0` 通过 / `1` 代码错误 / `2` 环境缺失
- 依赖库：`Adafruit GFX Library`、`Adafruit SSD1306`
- 每个 PR 由 GitHub Actions 自动编译（[.github/workflows/arduino-compile.yml](.github/workflows/arduino-compile.yml)）
- **受限网络的云端 Agent 容器**（`downloads.arduino.cc` 等官方源被拦截、GitHub 放行）：
  先运行 `./scripts/setup-cloud-env.sh`，它把 arduino-cli、ESP32 core、依赖库、ctags
  全部改从 GitHub Releases 获取（已在 Claude Code Cloud 实测通过编译）

## 9. 如何本地烧录

```bash
# 安装 arduino-cli 后：
./scripts/compile.sh --install-deps      # 首次准备环境
./scripts/upload.sh /dev/ttyUSB0         # 烧录（Windows 用 COM 口）
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200   # 看串口日志
```

不用脚本时的原生 arduino-cli 命令（与 scripts/ 等价）：

```bash
arduino-cli core install esp32:esp32 \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306"
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/parking-system   # 编译
arduino-cli upload  -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 firmware/parking-system   # 烧录
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200                 # 串口监视
```

也可以用 Arduino IDE：打开 `firmware/parking-system/parking-system.ino`，
开发板选 "ESP32 Dev Module"，安装上述 2 个库后编译上传。
`scripts/upload.sh` 仅限本地使用，云端 Agent 无串口硬件。

## 10. 如何配置 Wi-Fi

```bash
cd firmware/parking-system/config
cp WifiCredentials.example.h WifiCredentials.h
# 编辑 WifiCredentials.h 填入真实 SSID/密码（该文件已被 .gitignore 忽略，不会入库）
```

- 不创建该文件也能编译运行：固件自动开启热点 `ESP32-Parking`
  （密码 `parking123`，可在 `Settings.h` 修改），手机连热点后访问
  `http://192.168.4.1/`
- STA 连接成功后串口会打印分配到的 IP，浏览器访问该 IP 即可；
  运行中掉线会自动等待重连，超时后降级到 AP 热点
- ⚠️ **安全边界**：Web 的"清零累计收入"接口（`POST /api/reset`）没有认证，
  同一网络内任何设备都能清零统计——仅限本地演示网络使用，绝不可暴露公网；
  可在 `Settings.h` 置 `ENABLE_WEB_MANUAL_CONTROL 0` 关闭（接口返回 403，
  仪表盘变为只读）

## 11. 如何调整计费单价

计费参数都在 `config/Settings.h`，改完重新编译烧录即可：

| 参数 | 含义 | 默认值 |
| --- | --- | --- |
| `PARKING_FREE_PERIOD_SEC` | 免费时长（秒），短暂停靠不计费 | 60 |
| `PARKING_RATE_PER_MIN_CENTS` | 每分钟单价（分），50 = ¥0.50/min | 50 |
| `CURRENCY_SYMBOL` | 网页/JSON 货币符号 | `¥` |
| `MAX_SESSION_LOG` | 保留并展示的最近停车记录条数 | 5 |

计费规则（按**总停留时长**向上取整，不扣除免费时长）：停留时长 ≤ 免费时长 → 0；
否则 应付分钟 = ⌈总停留分钟⌉，费用 = 应付分钟 × 单价。
例：免费 60s、单价 ¥0.50/min 时，停 61s 按 2 分钟计 = ¥1.00。
金额全程用整数"分"运算，避免浮点误差。
"清零累计收入"按钮（`POST /api/reset`）会清空收入与最近记录，并把在场车辆
计时归零（相当于本轮演示重新开始）。
⚠️ **"收费"仅指本地费用计算与展示，不接任何真实支付/收款。**

## 12. 如何进行硬件实测

按 [docs/hardware-wiring.md](docs/hardware-wiring.md) 的"上电自检顺序"逐模块接线，
对照 [tests/README.md](tests/README.md) 的测试清单逐项验证，
把串口日志/照片按 [docs/cloud-workflow.md](docs/cloud-workflow.md) 的反馈格式贴回 PR。
实测后通常需要校准 `Settings.h`：红外触发电平、去抖时间、OLED 地址、
计费单价与免费时长。

**最小验收标准**（MVP 达标线，完整清单见 tests/README.md）：

- [ ] Arduino CLI 编译通过（本地或 CI 绿勾）
- [ ] 烧录 ESP32 成功，串口 115200 能看到启动 banner 与各模块 `init OK`
- [ ] 串口 / OLED / 网页能看到每个车位 FREE / OCCUPIED 状态与累计收入
- [ ] 遮挡车位传感器 → 识别入场（1 短响、开始计时）；移开 → 离场计费
      （2 短响，费用 = 时长 × 单价，免费时长内为 0）
- [ ] 去抖生效：传感器抖动不会导致车位状态频繁跳变或误计一次进出

## 13. 答辩演示流程

见 [docs/demo-script.md](docs/demo-script.md)：6 个场景（启动自检、车辆入场计时、
车辆离场计费、满位提示、Web 远程查看与清零、复位收尾）+ 现场故障预案。

**时间紧 / 要交演示视频**：把 `config/Settings.h` 的 `ENABLE_SIM_MODE` 置 1 重新烧录，
网页会出现「车辆驶入 / 驶离」按钮，无需红外传感器即可跑通完整闭环——
速成外观（A4 / 卡纸纸模型）、录视频脚本与 2 天时间表见
[docs/model-build.md](docs/model-build.md)。

## 14. Phase 2 计划

- 火焰 / 烟雾检测报警，风扇自动联动（引脚与 `AlertPattern::ALARM` 已预留）
- 对射式过车检测（激光 + U 型光电）做进出场计数
- 道闸放行只靠红外车辆识别，**不做 RFID 刷卡鉴权**（套件中的 RC522 闲置不用）
- 计费记录数据持久化与导出（视进度取舍）
- ParkingManager / BillingService 主机端单元测试

## 仓库结构

```
├── README.md / CLAUDE.md / AGENTS.md     # 首页 / 开发 Agent 守则 / 审查 Agent 守则
├── docs/                                 # 架构、引脚、接线、工作流、演示、论文大纲
├── firmware/parking-system/              # Arduino 固件（.ino + config/ + src/ + web/）
├── scripts/                              # compile.sh（云端/CI/本地）、upload.sh（仅本地）
├── .github/workflows/arduino-compile.yml # PR 自动编译检查
└── tests/                                # 测试说明与硬件测试清单
```

## License

MIT（见 [LICENSE](LICENSE)）
