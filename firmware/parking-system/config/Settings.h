#pragma once
#include <Arduino.h>

// =====================================================================
// Settings.h — 全部功能开关与可调参数的唯一定义处
//
// 规则：业务代码中禁止散落魔法数字；阈值、时长、单价都在这里改。
// 硬件实测时（接线、传感器逻辑、计费单价标定）只需要改这个文件。
// =====================================================================

// ---------------------------------------------------------------
// 功能开关（1 = 启用，0 = 关闭；关闭后对应模块应仍可编译）
// ---------------------------------------------------------------
#define ENABLE_DISPLAY       1
#define ENABLE_WEB           1
#define ENABLE_BUZZER        1
#define ENABLE_DEBUG_SERIAL  1

// Web 管理接口（POST /api/reset：清零累计收入与最近记录，并把在场车辆计时
// 归零，相当于本轮演示重新开始）。
// 安全边界：该接口没有认证，同一网络内任何客户端都能清零统计，
// 仅限本地演示网络使用，绝不可暴露到公共网络；置 0 后接口返回 403，
// 仪表盘只剩只读状态展示。
#define ENABLE_WEB_MANUAL_CONTROL 1

// ---------------------------------------------------------------
// 串口
// ---------------------------------------------------------------
constexpr uint32_t SERIAL_BAUD = 115200;

// ---------------------------------------------------------------
// 车位（红外传感器）
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
// 计费（仅本地计算与展示，不是真实支付/收款）
//
// 计费模型（按分钟，向上取整；按"总停留时长"取整，不扣除免费时长）：
//   停留时长 <= 免费时长          -> 0
//   否则 应付分钟 = ceil(总停留分钟)，费用 = 应付分钟 × 每分钟单价
//   例：免费 60s、单价 50 分时，停 61s 按 2 分钟计 = ¥1.00
// 金额一律用「分」做整数运算，避免浮点误差，展示时再换算成「元.角分」。
// 下面是演示用单价，本地可随意调整。
// ---------------------------------------------------------------
constexpr uint32_t PARKING_FREE_PERIOD_SEC    = 60;  // 前 N 秒免费（短暂停靠不计费）
constexpr uint32_t PARKING_RATE_PER_MIN_CENTS = 50;  // 每分钟单价（分）；50 = ¥0.50/min（演示值）
constexpr const char* CURRENCY_SYMBOL         = "¥"; // 网页/JSON 货币符号（OLED 用纯数字，不带符号）
constexpr uint8_t  MAX_SESSION_LOG            = 5;    // 保留并展示的最近停车记录条数

// ---------------------------------------------------------------
// 蜂鸣器节奏（默认有源蜂鸣器：给电平就响）
// 若使用无源蜂鸣器，需要改用 PWM 驱动，本期不做。
// ---------------------------------------------------------------
constexpr int      BUZZER_ACTIVE_LEVEL = HIGH;  // 模块若为低电平触发，改成 LOW
constexpr uint32_t BEEP_SHORT_MS       = 80;    // 短响时长（入场 1 次 / 出场 2 次）
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
