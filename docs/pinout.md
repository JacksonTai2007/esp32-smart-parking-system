# 引脚分配表（Pinout）

代码中的唯一权威定义在 `firmware/parking-system/config/Pins.h`，
本表与其保持一致；修改任意一处必须同步另一处和 README。

> 本期方案：红外车位传感器做车辆识别 + 车位管理 + 按时长计费，并加入 SG90
> 出入口道闸（进出场自动抬杆 + 网页手动开 / 落，可由 `ENABLE_GATE` 关闭）。
> RFID 刷卡、HC-SR04 入口超声波仍未纳入。

| 功能 | 模块 | ESP32 引脚 | 备注 |
| --- | --- | --- | --- |
| I2C SDA | OLED SSD1306 | GPIO 21 | I2C |
| I2C SCL | OLED SSD1306 | GPIO 22 | I2C |
| 蜂鸣器 | Buzzer | GPIO 14 | 数字输出 |
| 出入口道闸 | SG90 舵机 | GPIO 13 | PWM 输出（ESP32 LEDC 驱动） |
| 车位 1 | 红外传感器 | GPIO 32 | 数字输入 |
| 车位 2 | 红外传感器 | GPIO 33 | 数字输入 |
| 车位 3 | 红外传感器 | GPIO 34 | 输入专用 |
| 车位 4 | 红外传感器 | GPIO 35 | 输入专用 |
| 火焰报警 | 火焰传感器 | GPIO 27 | Phase 2 预留 |
| 烟雾检测 | MQ 烟雾模块 | GPIO 36 | Phase 2 预留，模拟输入 |
| 风扇控制 | MOSFET/继电器 | GPIO 16 | Phase 2 预留 |

## 注意事项（接线前必读）

1. **OLED 使用 3.3V 供电**，SDA/SCL 接 GPIO 21/22；I2C 地址多数为 0x3C，
   少数 0x3D（改 `config/Settings.h` 的 `OLED_I2C_ADDRESS`）。
2. **外接电源时 ESP32、各传感器必须共地（GND 连通）**，
   否则信号电平没有共同参考，行为随机异常。
3. **GPIO 34、35、36（及 39）是输入专用脚**：不能作为输出，
   也没有内部上拉/下拉。车位红外模块自带推挽输出，可直接接入；
   如换成开漏输出的模块需外接上拉电阻。
4. **启动敏感（strapping）引脚 GPIO 0/2/5/12/15**：本期方案均未使用，
   扩展功能时优先避开这些脚。
5. **实际接线后需根据模块的高低电平逻辑调整 `config/Settings.h`**：
   - 红外车位模块：常见为"检测到物体输出 LOW"（`SLOT_OCCUPIED_LEVEL = LOW`），
     若你的模块相反，改为 `HIGH`
   - 蜂鸣器模块：默认高电平触发（`BUZZER_ACTIVE_LEVEL = HIGH`），
     低电平触发的模块改为 `LOW`
6. 只接 2~3 个车位时，把 `Settings.h` 的 `TOTAL_PARKING_SLOTS` 改成实际数量，
   按顺序占用 GPIO 32→33→34 引脚。
7. GPIO 36 为 ADC1 通道，Phase 2 的 MQ 烟雾模块 AO 输出接它做模拟采样；
   使用 Wi-Fi 时 ADC2 不可用，因此模拟输入只规划在 ADC1 引脚上。
8. **SG90 道闸舵机**：信号线（橙）接 GPIO 13，由 ESP32 LEDC 生成 50Hz PWM 驱动
   （不需第三方库）。舵机红线接 5V、棕线接 GND；多个/大舵机抖动或复位时，
   改用独立 5V 供电并与 ESP32 **共地**。开 / 合角度与抬杆保持时长在
   `config/Settings.h`（`GATE_SERVO_OPEN_DEG` / `GATE_SERVO_CLOSED_DEG` /
   `GATE_OPEN_HOLD_MS`）；方向相反就把两个角度对调。不接舵机也能运行，
   网页 / OLED 仍显示道闸状态。
