#pragma once
#include <Arduino.h>
#include "../config/Settings.h"

// =====================================================================
// ParkingTypes.h — 全局共享的状态枚举与数据结构
//
// 本期方案：红外车位识别 + 车位管理 + 按时长计费（无 RFID / 无闸机）。
// =====================================================================

// 蜂鸣器提示模式
enum class AlertPattern : uint8_t {
    NONE,
    ENTER,  // 车辆入场：短响 1 次
    EXIT,   // 车辆离场（已计费）：短响 2 次
    FULL,   // 车位已满：长响 1 次
    ALARM,  // 报警：循环间隔响（Phase 2 烟雾/火焰预留）
};

// 单个车位
struct ParkingSlot {
    uint8_t  id           = 0;      // 从 1 开始编号
    bool     occupied     = false;
    uint32_t lastChangeMs = 0;      // 最近一次状态变化时刻（millis）
};

// 一条已结算的停车记录（车辆离场时生成）
struct ParkingSession {
    uint8_t  slotId     = 0;        // 车位号（从 1 开始）
    uint32_t durationMs = 0;        // 停留时长
    uint32_t feeCents   = 0;        // 费用（分）
};

// 系统状态快照：ParkingManager 生成，OLED 与 Web 只读消费
struct ParkingStatus {
    uint8_t  totalSlots    = 0;
    uint8_t  occupiedSlots = 0;
    uint8_t  freeSlots     = 0;
    bool     slotOccupied[MAX_PARKING_SLOTS]   = {false};
    uint32_t slotDurationMs[MAX_PARKING_SLOTS] = {0};  // 占用车位的当前停留时长，空闲为 0

    uint32_t totalRevenueCents = 0;   // 自启动/重置以来的累计收入（分）
    uint32_t sessionCount      = 0;   // 已结算的停车次数

    ParkingSession recent[MAX_SESSION_LOG];  // 最近若干条记录，recent[0] 最新
    uint8_t        recentCount = 0;

    char     lastMessage[48] = "System ready";
    uint32_t uptimeMs        = 0;
};

// ---------------------------------------------------------------
// 展示用格式化助手（OLED 与 Web 复用，避免重复实现）
// ---------------------------------------------------------------

// 金额：分 -> "12.50"（不含货币符号；OLED 直接用，网页前面再拼符号）
inline void formatMoneyCents(char* buf, size_t n, uint32_t cents) {
    snprintf(buf, n, "%lu.%02lu",
             (unsigned long)(cents / 100), (unsigned long)(cents % 100));
}

// 时长：毫秒 -> "MM:SS"（不足 1 小时）或 "H:MM:SS"
inline void formatDurationMs(char* buf, size_t n, uint32_t ms) {
    const uint32_t totalSec = ms / 1000;
    const uint32_t h = totalSec / 3600;
    const uint32_t m = (totalSec % 3600) / 60;
    const uint32_t s = totalSec % 60;
    if (h > 0) {
        snprintf(buf, n, "%lu:%02lu:%02lu",
                 (unsigned long)h, (unsigned long)m, (unsigned long)s);
    } else {
        snprintf(buf, n, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
    }
}
