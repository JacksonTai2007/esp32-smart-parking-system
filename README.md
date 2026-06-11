# 基于 ESP32 的智能停车场管理系统设计与实现

ESP32 Smart Parking System — 大学毕业设计项目

[![Arduino Compile](https://github.com/JacksonTai2007/esp32-smart-parking-system/actions/workflows/arduino-compile.yml/badge.svg)](https://github.com/JacksonTai2007/esp32-smart-parking-system/actions/workflows/arduino-compile.yml)

## 1. 项目简介

以 ESP32 为主控的智能停车场管理系统：超声波检测入口来车，RFID 刷卡鉴权，
SG90 舵机模拟闸机起落，红外传感器实时监测车位占用，OLED 与本地 Web 页面
同步展示剩余车位与系统状态，蜂鸣器提供声音反馈。

核心演示流程：

```
车辆靠近入口 → 超声波检测 → OLED 显示"请刷卡" → RFID 合法刷卡
→ 蜂鸣器提示成功 → SG90 舵机开闸 → 车位传感器更新 → OLED/Web 显示剩余车位
```

## 2. 毕设定位

- 课题：基于 ESP32 的智能停车场管理系统设计与实现
- 当前阶段：**Phase 1 MVP**（最小可演示闭环），论文大纲见 [docs/thesis-outline.md](docs/thesis-outline.md)
- 项目特色之一：采用**云端 AI Agent 协作开发流程**（见第 7 节），
  开发过程本身可作为论文的工程方法章节素材
- 答辩演示脚本见 [docs/demo-script.md](docs/demo-script.md)

## 3. 功能列表

### Phase 1（本期已实现，待硬件实测）
- ✅ RC522 RFID 刷卡识别 + 白名单鉴权（串口打印每张卡 UID，便于登记）
- ✅ SG90 舵机闸机：开闸 → 保持 5 秒 → 自动关闸，全程非阻塞
- ✅ HC-SR04 超声波入口来车检测（滞回 + 连续确认抗抖动）
- ✅ 2~4 路红外车位检测（软件去抖、触发电平可配置）
- ✅ OLED SSD1306 实时状态显示
- ✅ 蜂鸣器提示（成功 1 短响 / 无效卡 3 短响 / 满位 1 长响）
- ✅ 本地 Web 仪表盘 + JSON 状态 API + 手动开关闸（STA 失败自动开热点兜底）
- ✅ 主业务状态机（IDLE → 来车 → 等待刷卡 → 放行/拒绝/满位）

### Phase 2（规划，引脚已预留）
火焰/烟雾报警与风扇联动（GPIO 27/36/16）、入场记录与计费、数据持久化等。

## 4. 硬件清单

| 器件 | 数量 | 说明 |
| --- | --- | --- |
| ESP32 DevKit（WROOM-32） | 1 | 主控 |
| RC522 RFID 模块 + IC 卡/钥匙扣 | 1 套 | 刷卡鉴权（**3.3V 供电**） |
| SG90 舵机 | 1 | 模拟闸机 |
| HC-SR04 超声波模块 | 1 | 入口来车检测 |
| 红外避障传感器 | 2~4 | 车位占用检测 |
| OLED SSD1306 128×64（I2C） | 1 | 状态显示 |
| 有源蜂鸣器模块 | 1 | 声音提示 |
| 5V 外接电源 | 1 | 舵机独立供电（与 ESP32 共地） |
| 电阻 1kΩ + 2kΩ | 各 1 | HC-SR04 Echo 分压 |
| 面包板、杜邦线、小车模型 | 若干 | 搭建与演示 |

## 5. 接线表

完整接线说明与排查方法见 [docs/pinout.md](docs/pinout.md) 和
[docs/hardware-wiring.md](docs/hardware-wiring.md)，代码权威定义在
[`firmware/parking-system/config/Pins.h`](firmware/parking-system/config/Pins.h)。

| 功能 | 模块 | ESP32 引脚 | 备注 |
| --- | --- | --- | --- |
| I2C SDA / SCL | OLED SSD1306 | GPIO 21 / 22 | I2C |
| RFID SCK / MISO / MOSI | RC522 | GPIO 18 / 19 / 23 | SPI |
| RFID SS / RST | RC522 | GPIO 5 / 4 | CS、Reset |
| 闸机舵机 | SG90 | GPIO 13 | 外接 5V，共地 |
| 蜂鸣器 | Buzzer | GPIO 14 | 数字输出 |
| 超声波 Trig / Echo | HC-SR04 | GPIO 25 / 26 | Echo 分压到 3.3V |
| 车位 1~4 | 红外传感器 | GPIO 32 / 33 / 34 / 35 | 34/35 输入专用 |
| 火焰 / 烟雾 / 风扇 | Phase 2 预留 | GPIO 27 / 36 / 16 | 本期不接 |

⚠️ 关键安全项：RC522 只能接 3.3V；SG90 外接 5V 并与 ESP32 共地；
HC-SR04 Echo 建议 1kΩ+2kΩ 分压。

**默认硬件假设**（实测不符时只需改 `config/Settings.h`）：开发板为 ESP32 DevKit
（WROOM-32）；车位传感器为红外避障模块，**检测到物体输出 LOW**
（`SLOT_OCCUPIED_LEVEL = LOW`）；蜂鸣器为高电平触发的有源模块；车位数 4
（可配 2~4）；串口波特率 **115200**。

## 6. 软件架构

模块化固件 + 非阻塞 `millis()` 状态机，详见 [docs/architecture.md](docs/architecture.md)。

| 模块 | 职责 |
| --- | --- |
| `ParkingStateMachine` | 主业务状态机 |
| `GateController` | SG90 舵机闸机控制（自动关闸） |
| `RfidService` | RC522 读卡 + 白名单 |
| `UltrasonicService` | 入口车辆靠近检测 |
| `SlotManager` | 车位状态与剩余车位统计 |
| `DisplayService` | OLED 显示 |
| `AlertService` | 蜂鸣器节奏 |
| `WebDashboard` | 网页 + JSON API + Wi-Fi 管理 |

约定：GPIO 只在 `config/Pins.h` 定义；阈值/角度/时长/功能开关只在
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
- 依赖库：`ESP32Servo`、`MFRC522`、`Adafruit GFX Library`、`Adafruit SSD1306`
- 每个 PR 由 GitHub Actions 自动编译（[.github/workflows/arduino-compile.yml](.github/workflows/arduino-compile.yml)）
- **受限网络的云端 Agent 容器**（`downloads.arduino.cc` 等官方源被拦截、GitHub 放行）：
  先运行 `./scripts/setup-cloud-env.sh`，它把 arduino-cli、ESP32 core、依赖库、ctags
  全部改从 GitHub Releases 获取（已在 Claude Code Cloud 实测通过编译）

## 9. 如何本地烧录

### 方式一（推荐）：Arduino IDE

仓库布局完全遵循 Arduino 草图规范（`.ino` 与文件夹同名、`src/` 子目录自动
递归编译），IDE 直接打开即可，无需调整结构。建议使用 **Arduino IDE 2.x**
（其底层与本项目 CI 用的 arduino-cli 是同一套构建引擎）。

1. **装板卡支持**：File → Preferences → Additional boards manager URLs 填入
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`，
   然后 Boards Manager 搜索 `esp32`（by Espressif Systems）安装
   （CI 验证版本为 3.3.10，装 3.x 即可）
2. **装库**：Library Manager 依次安装 `ESP32Servo`、`MFRC522`
   （by GithubCommunity）、`Adafruit GFX Library`、`Adafruit SSD1306`
   （提示安装依赖 Adafruit BusIO 时选 Install All）
3. **打开工程**：File → Open 选择
   `firmware/parking-system/parking-system.ino`
4. **配置 Wi-Fi（可选）**：把 `config/WifiCredentials.example.h` 复制为
   `config/WifiCredentials.h` 填入账号（任意编辑器改即可）；
   不配置则自动走 AP 热点模式
5. **选板与端口**：Tools → Board 选 **ESP32 Dev Module**（默认参数即可），
   Tools → Port 选对应串口
6. **编译/烧录**：Verify（✓）编译，Upload（→）烧录；个别开发板若出现
   `Connecting...` 卡住，按住板上 **BOOT** 键再松开即可进入下载模式
7. **看日志**：Serial Monitor，波特率选 **115200**

调参只需改 `config/Settings.h`（阈值、角度、电平、白名单），改完重新
Verify + Upload。

### 方式二：arduino-cli（脚本封装）

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
arduino-cli lib install "ESP32Servo" "MFRC522" "Adafruit GFX Library" "Adafruit SSD1306"
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/parking-system   # 编译
arduino-cli upload  -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 firmware/parking-system   # 烧录
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200                 # 串口监视
```

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
- ⚠️ **安全边界**：Web 手动开/关闸接口没有认证，同一网络内任何设备都能
  触发物理闸机——仅限本地演示网络使用，绝不可暴露公网；
  可在 `Settings.h` 置 `ENABLE_WEB_MANUAL_GATE_CONTROL 0` 关闭（接口返回 403，
  仪表盘变为只读）

## 11. 如何添加 RFID 白名单

1. 烧录后打开串口监视器（115200）
2. 刷卡，记录日志：`[RFID] New card UID: A1:B2:C3:D4`
3. 把 UID 填入 `config/Settings.h` 的 `RFID_WHITELIST` 数组（替换占位卡号）
4. 重新编译烧录

## 12. 如何进行硬件实测

按 [docs/hardware-wiring.md](docs/hardware-wiring.md) 的"上电自检顺序"逐模块接线，
对照 [tests/README.md](tests/README.md) 的 12 项测试清单逐项验证，
把串口日志/照片按 [docs/cloud-workflow.md](docs/cloud-workflow.md) 的反馈格式贴回 PR。
实测后通常需要校准 `Settings.h`：超声波阈值、红外触发电平、舵机角度、
蜂鸣器电平、OLED 地址。

**最小验收标准**（MVP 达标线，完整清单见 tests/README.md）：

- [ ] Arduino CLI 编译通过（本地或 CI 绿勾）
- [ ] 烧录 ESP32 成功，串口 115200 能看到启动 banner 与各模块 `init OK`
- [ ] 串口 / OLED / 网页能看到每个车位 FREE / OCCUPIED 状态
- [ ] 遮挡、移开车位传感器时状态变化正确
- [ ] 去抖生效：传感器抖动不会导致车位状态频繁跳变

## 13. 答辩演示流程

见 [docs/demo-script.md](docs/demo-script.md)：6 个场景（启动、合法卡入场、
无效卡拒绝、满位处理、Web 远程控制、复位收尾）+ 现场故障预案。

## 14. Phase 2 计划

- 火焰 / 烟雾检测报警，风扇自动联动（引脚与 `AlertPattern::ALARM` 已预留）
- 入场/出场记录与简单计费模型
- 出口闸机与双向流程
- 状态机主机端单元测试
- 数据持久化与上位机/云平台对接（视进度取舍）

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
