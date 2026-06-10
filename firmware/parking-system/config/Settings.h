#pragma once
#include <Arduino.h>

// =====================================================================
// Settings.h — 全部功能开关与可调参数的唯一定义处
//
// 规则：业务代码中禁止散落魔法数字；阈值、时长、角度都在这里改。
// 硬件实测时（接线、传感器逻辑、阈值校准）只需要改这个文件。
// =====================================================================

// ---------------------------------------------------------------
// 功能开关（1 = 启用，0 = 关闭；关闭后对应模块应仍可编译）
// ---------------------------------------------------------------
#define ENABLE_RFID          1
#define ENABLE_DISPLAY       1
#define ENABLE_WEB           1
#define ENABLE_BUZZER        1
#define ENABLE_DEBUG_SERIAL  1

// ---------------------------------------------------------------
// 串口
// ---------------------------------------------------------------
constexpr uint32_t SERIAL_BAUD = 115200;

// ---------------------------------------------------------------
// 车位
// ---------------------------------------------------------------
constexpr uint8_t MAX_PARKING_SLOTS   = 4;  // 固件支持的上限（与 Pins.h 数组长度一致）
constexpr uint8_t TOTAL_PARKING_SLOTS = 4;  // 实际接线的车位数，可设 2~4
static_assert(TOTAL_PARKING_SLOTS >= 2 && TOTAL_PARKING_SLOTS <= MAX_PARKING_SLOTS,
              "TOTAL_PARKING_SLOTS must be 2..4");

// 红外避障模块常见逻辑：检测到物体时输出 LOW。
// 本地实测时若相反，把这里改成 HIGH 即可。
constexpr int      SLOT_OCCUPIED_LEVEL = LOW;
constexpr uint32_t SLOT_DEBOUNCE_MS    = 150;  // 车位状态去抖时间

// ---------------------------------------------------------------
// 闸机舵机（SG90）
// ---------------------------------------------------------------
constexpr int      GATE_CLOSED_ANGLE   = 0;     // 关闸角度
constexpr int      GATE_OPEN_ANGLE     = 90;    // 开闸角度（本地按机械安装校准）
constexpr uint32_t GATE_MOTION_TIME_MS = 700;   // SG90 估算行程时间（无反馈，按时间推进状态）
constexpr uint32_t GATE_OPEN_HOLD_MS   = 5000;  // 开闸保持时间，到时自动关闸
constexpr int      SERVO_MIN_PULSE_US  = 500;   // SG90 常用脉宽范围
constexpr int      SERVO_MAX_PULSE_US  = 2400;

// ---------------------------------------------------------------
// 入口超声波（HC-SR04）
// ---------------------------------------------------------------
constexpr uint32_t ULTRASONIC_SAMPLE_INTERVAL_MS = 100;    // 采样周期
constexpr uint32_t ULTRASONIC_TIMEOUT_US         = 25000;  // pulseIn 超时（约 4.3 m），防长阻塞
constexpr float    ULTRASONIC_DETECT_CM          = 20.0f;  // 距离小于该值判定来车
constexpr float    ULTRASONIC_RELEASE_CM         = 30.0f;  // 距离大于该值判定车辆离开（滞回）
constexpr uint8_t  ULTRASONIC_CONFIRM_SAMPLES    = 3;      // 连续 N 次有效检测才确认，抗抖动

// ---------------------------------------------------------------
// RFID（RC522）
// ---------------------------------------------------------------
constexpr uint32_t RFID_READ_COOLDOWN_MS = 1500;  // 同一张卡贴着不动时的重复读取间隔
constexpr size_t   RFID_UID_MAX_LEN      = 32;    // UID 字符串缓冲（支持 4/7/10 字节 UID）

// 白名单：UID 为大写十六进制、冒号分隔。
// 下面是占位示例。本地烧录后刷卡，串口会打印
//   [RFID] New card UID: XX:XX:XX:XX
// 把打印出来的 UID 替换/追加到这里即可。
constexpr const char* RFID_WHITELIST[] = {
    "DE:AD:BE:EF",  // 占位示例卡 1（请替换为真实卡号）
    "12:34:56:78",  // 占位示例卡 2（请替换为真实卡号）
};
constexpr size_t RFID_WHITELIST_SIZE = sizeof(RFID_WHITELIST) / sizeof(RFID_WHITELIST[0]);

// ---------------------------------------------------------------
// 蜂鸣器节奏（默认有源蜂鸣器：给电平就响）
// 若使用无源蜂鸣器，需要改用 PWM 驱动，本期不做。
// ---------------------------------------------------------------
constexpr int      BUZZER_ACTIVE_LEVEL = HIGH;  // 模块若为低电平触发，改成 LOW
constexpr uint32_t BEEP_SHORT_MS       = 80;    // 短响时长（刷卡成功 1 次 / 无效卡 3 次）
constexpr uint32_t BEEP_GAP_MS         = 120;   // 多次短响之间的间隔
constexpr uint32_t BEEP_LONG_MS        = 600;   // 长响时长（满位提示）
constexpr uint32_t ALARM_INTERVAL_MS   = 300;   // 报警间隔响（Phase 2 烟雾/火焰预留）

// ---------------------------------------------------------------
// OLED 显示（SSD1306 I2C）
// ---------------------------------------------------------------
constexpr uint8_t  OLED_I2C_ADDRESS   = 0x3C;  // 常见地址 0x3C，部分模块为 0x3D
constexpr uint8_t  OLED_WIDTH         = 128;
constexpr uint8_t  OLED_HEIGHT        = 64;
constexpr uint32_t DISPLAY_REFRESH_MS = 250;   // 刷新周期

// ---------------------------------------------------------------
// 主状态机时序
// ---------------------------------------------------------------
constexpr uint32_t VEHICLE_DETECTED_DWELL_MS = 400;    // "检测到车辆"提示停留时间
constexpr uint32_t CARD_WAIT_TIMEOUT_MS      = 15000;  // 等待刷卡超时，回到空闲
constexpr uint32_t REJECT_MESSAGE_MS         = 2000;   // 无效卡提示停留时间

// ---------------------------------------------------------------
// Wi-Fi / Web
// 真实 STA 账号密码放在 config/WifiCredentials.h（已被 .gitignore 忽略，
// 模板见 WifiCredentials.example.h）。仓库中绝不提交真实密码。
// ---------------------------------------------------------------
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;  // STA 连接超时

// STA 连接失败或没有提供 WifiCredentials.h 时，是否自动开热点（AP）兜底。
// 答辩现场没有可用 Wi-Fi 时，手机直接连 ESP32 热点即可访问网页。
#define WIFI_AP_FALLBACK 1
constexpr const char* WIFI_AP_SSID     = "ESP32-Parking";
constexpr const char* WIFI_AP_PASSWORD = "parking123";  // 仅演示热点的本机密码，不是任何真实账号凭据
constexpr const char* WIFI_HOSTNAME    = "esp32-parking";
constexpr uint16_t    WEB_SERVER_PORT  = 80;

// ---------------------------------------------------------------
// 调试日志宏：关闭 ENABLE_DEBUG_SERIAL 后日志代码不参与编译
// ---------------------------------------------------------------
#if ENABLE_DEBUG_SERIAL
#define LOG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#define LOG_PRINTLN(x)   Serial.println(x)
#else
#define LOG_PRINTF(...)  do {} while (0)
#define LOG_PRINTLN(x)   do {} while (0)
#endif
