# 引脚分配表（Pinout）

代码中的唯一权威定义在 `firmware/parking-system/config/Pins.h`，
本表与其保持一致；修改任意一处必须同步另一处和 README。

| 功能 | 模块 | ESP32 引脚 | 备注 |
| --- | --- | --- | --- |
| I2C SDA | OLED SSD1306 | GPIO 21 | I2C |
| I2C SCL | OLED SSD1306 | GPIO 22 | I2C |
| RFID SCK | RC522 | GPIO 18 | SPI |
| RFID MISO | RC522 | GPIO 19 | SPI |
| RFID MOSI | RC522 | GPIO 23 | SPI |
| RFID SS/SDA | RC522 | GPIO 5 | SPI CS |
| RFID RST | RC522 | GPIO 4 | Reset |
| 闸机舵机 | SG90 | GPIO 13 | 外接 5V，共地 |
| 蜂鸣器 | Buzzer | GPIO 14 | 数字输出 |
| 入口超声波 Trig | HC-SR04 | GPIO 25 | 数字输出 |
| 入口超声波 Echo | HC-SR04 | GPIO 26 | 建议分压到 3.3V |
| 车位 1 | 红外传感器 | GPIO 32 | 数字输入 |
| 车位 2 | 红外传感器 | GPIO 33 | 数字输入 |
| 车位 3 | 红外传感器 | GPIO 34 | 输入专用 |
| 车位 4 | 红外传感器 | GPIO 35 | 输入专用 |
| 火焰报警 | 火焰传感器 | GPIO 27 | Phase 2 预留 |
| 烟雾检测 | MQ 烟雾模块 | GPIO 36 | Phase 2 预留，模拟输入 |
| 风扇控制 | MOSFET/继电器 | GPIO 16 | Phase 2 预留 |

## 注意事项（接线前必读）

1. **RC522 使用 3.3V 供电，不要接 5V**，否则可能烧毁模块。
2. **SG90 舵机建议外接 5V 电源，不要直接吃 ESP32 的 3.3V**；
   ESP32 板载稳压器带不动舵机堵转电流，会导致复位甚至损坏。
3. **HC-SR04 的 Echo 输出可能是 5V**，进入 ESP32 前建议用电阻分压到 3.3V
   （如 1kΩ + 2kΩ，见 docs/hardware-wiring.md）。Trig 由 ESP32 输出 3.3V
   即可触发，无需处理。
4. **外接电源时 ESP32、舵机、各传感器必须共地（GND 连通）**，
   否则信号电平没有共同参考，行为随机异常。
5. **GPIO 34、35、36（及 39）是输入专用脚**：不能作为输出，
   也没有内部上拉/下拉。车位红外模块自带推挽输出，可直接接入；
   如换成开漏输出的模块需外接上拉电阻。
6. **启动敏感（strapping）引脚 GPIO 0/2/5/12/15**：本表仅在 GPIO 5 上挂
   RC522 的 SPI CS（RC522 不会在上电时拉低它，属常见安全用法），
   其余 strapping 脚均未使用；扩展功能时优先避开这些脚。
7. **实际接线后需根据模块的高低电平逻辑调整 `config/Settings.h`**：
   - 红外车位模块：常见为"检测到物体输出 LOW"（`SLOT_OCCUPIED_LEVEL = LOW`），
     若你的模块相反，改为 `HIGH`
   - 蜂鸣器模块：默认高电平触发（`BUZZER_ACTIVE_LEVEL = HIGH`），
     低电平触发的模块改为 `LOW`
8. GPIO 36 为 ADC1 通道，Phase 2 的 MQ 烟雾模块 AO 输出接它做模拟采样；
   使用 Wi-Fi 时 ADC2 不可用，因此模拟输入只规划在 ADC1 引脚上。
