# 硬件接线说明

引脚总表见 docs/pinout.md（代码权威定义：`config/Pins.h`）。
本文按模块给出具体接线、供电与排查要点。**接线前务必通读 pinout.md 的注意事项。**

## 供电架构

```
USB 5V ──► ESP32 DevKit（板载稳压出 3.3V）
                │ 3.3V ──► RC522、OLED、HC-SR04(VCC 5V 见下)、红外模块(3.3~5V 均可)
                │
外接 5V 电源 ──► SG90 舵机 VCC（红线）
     │
     └────────► GND 必须与 ESP32 GND 连通（共地！）
```

- **SG90 必须外接 5V**（手机充电头 + 接线端子、或 4 节电池盒均可），
  只把信号线（橙/黄）接 GPIO 13；直接用 ESP32 3.3V 带舵机会触发欠压复位
- **所有地必须连通**：ESP32 GND、外接 5V 电源 GND、各模块 GND 接在同一排
- HC-SR04 标准款 VCC 接 5V（ESP32 板上 VIN/5V 脚），Trig 可直接接 3.3V 信号

## 1. RC522 RFID（SPI）

| RC522 引脚 | 接到 | 备注 |
| --- | --- | --- |
| 3.3V | ESP32 3.3V | **绝对不要接 5V** |
| GND | GND | |
| SCK | GPIO 18 | |
| MISO | GPIO 19 | |
| MOSI | GPIO 23 | |
| SDA(SS) | GPIO 5 | SPI 片选 |
| RST | GPIO 4 | |
| IRQ | 不接 | 本项目轮询方式 |

排查：串口启动日志 `RC522 firmware version 0x91/0x92` 为正常；
`0x00` 或 `0xFF` 说明 SPI 接线/供电有问题。

## 2. SG90 舵机（闸机）

| SG90 线色 | 接到 |
| --- | --- |
| 棕（GND） | 公共 GND |
| 红（VCC） | 外接 5V |
| 橙（信号） | GPIO 13 |

机械安装：先烧录后上电，舵机会回到 0°（关闸位），再安装闸杆，
避免装好后第一次上电打杆。开闸角度在 `Settings.h` 的 `GATE_OPEN_ANGLE` 校准。

## 3. HC-SR04 超声波（入口检测）

| HC-SR04 引脚 | 接到 | 备注 |
| --- | --- | --- |
| VCC | 5V | |
| GND | GND | |
| Trig | GPIO 25 | 3.3V 触发没有问题 |
| Echo | **分压后** GPIO 26 | Echo 输出 5V，需降到 3.3V |

Echo 分压（1kΩ + 2kΩ）：

```
Echo ──[ 1kΩ ]──┬──► GPIO 26
                │
              [ 2kΩ ]
                │
               GND        5V × 2k/(1k+2k) ≈ 3.3V
```

若你的模块标注 3.3V 兼容（如 HC-SR04P 接 3.3V 供电），可直连，但请在 PR
反馈中注明实际型号。

## 4. 红外车位传感器 × 2~4

| 模块引脚 | 接到 |
| --- | --- |
| VCC | 3.3V（多数模块 3.3~5V 兼容） |
| GND | GND |
| OUT | 车位1→GPIO 32，车位2→GPIO 33，车位3→GPIO 34，车位4→GPIO 35 |

- 只接 2~3 个车位时，把 `Settings.h` 的 `TOTAL_PARKING_SLOTS` 改成实际数量，
  按顺序占用 32→33→34 引脚
- 常见红外避障模块**检测到物体输出 LOW**（板载 LED 亮）；
  若你的模块相反，把 `SLOT_OCCUPIED_LEVEL` 改为 `HIGH`
- 模块上的电位器可调检测距离，演示前调到稳定触发小车模型的距离

## 5. OLED SSD1306（I2C）

| OLED 引脚 | 接到 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

不亮时排查：I2C 地址多数为 0x3C，少数 0x3D（改 `Settings.h` 的
`OLED_I2C_ADDRESS`）；可先跑 I2C scanner 示例确认。

## 6. 蜂鸣器

| 模块引脚 | 接到 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| I/O | GPIO 14 |

默认按"高电平响"的**有源蜂鸣器**模块处理；若是低电平触发，
改 `Settings.h` 的 `BUZZER_ACTIVE_LEVEL = LOW`。无源蜂鸣器需 PWM 驱动，
本期不支持，购买时认准"有源/active buzzer"。

## 上电自检顺序（首次接线后）

1. 只接 ESP32 + OLED → 烧录 → 看 OLED 出画面、串口出 banner
2. 加 RC522 → 看版本号日志 → 刷卡看 UID
3. 加蜂鸣器 → 刷未登记卡应三短响
4. 加 HC-SR04 → 手掌靠近看 `vehicle detected` 日志
5. 加红外车位 → 逐个遮挡看车位日志
6. 最后接 SG90（外接电源）→ 完整流程演示

完整测试清单见 tests/README.md。
