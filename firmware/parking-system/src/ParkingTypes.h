#pragma once
#include <Arduino.h>
#include "../config/Settings.h"

// =====================================================================
// ParkingTypes.h — 全局共享的状态枚举与数据结构
// =====================================================================

// 闸机状态。SG90 没有位置反馈，OPENING/CLOSING 按 GATE_MOTION_TIME_MS 计时推进。
enum class GateState : uint8_t {
    CLOSED,
    OPENING,
    OPEN,
    CLOSING,
};

// 入口业务状态（主状态机）
enum class EntryState : uint8_t {
    IDLE,              // 空闲，等待车辆
    VEHICLE_DETECTED,  // 超声波检测到来车
    WAITING_FOR_CARD,  // 提示刷卡，等待 RFID（仅 ENABLE_RFID=1）
    ADMITTED,          // 放行：开闸欢迎入场（自动放行或刷卡通过都进入此状态）
    CARD_REJECTED,     // 非法卡，拒绝（仅 ENABLE_RFID=1）
    PARKING_FULL,      // 车位已满，不开闸
};

// 蜂鸣器提示模式
enum class AlertPattern : uint8_t {
    NONE,
    SUCCESS,  // 刷卡成功：短响 1 次
    REJECT,   // 无效卡：短响 3 次
    FULL,     // 满位：长响 1 次
    ALARM,    // 报警：循环间隔响（Phase 2 烟雾/火焰预留）
};

// 单个车位
struct ParkingSlot {
    uint8_t  id           = 0;      // 从 1 开始编号
    bool     occupied     = false;
    uint32_t lastChangeMs = 0;      // 最近一次状态变化时刻（millis）
};

// 一次刷卡事件（由 RfidService 产生，主状态机消费）
struct RfidEvent {
    bool authorized = false;
    char uid[RFID_UID_MAX_LEN] = {0};  // 大写十六进制，冒号分隔
};

// 系统状态快照：主状态机生成，OLED 与 Web 只读消费
struct ParkingStatus {
    uint8_t    totalSlots    = 0;
    uint8_t    occupiedSlots = 0;
    uint8_t    freeSlots     = 0;
    GateState  gateState     = GateState::CLOSED;
    EntryState entryState    = EntryState::IDLE;
    char       lastCardUid[RFID_UID_MAX_LEN] = "--";
    char       lastMessage[48]               = "System ready";
    bool       alarmActive   = false;
    uint32_t   uptimeMs      = 0;
    bool       slotOccupied[MAX_PARKING_SLOTS] = {false};
};

// 状态名（小写，直接用于 JSON API；OLED 也复用）
inline const char* gateStateName(GateState s) {
    switch (s) {
        case GateState::CLOSED:  return "closed";
        case GateState::OPENING: return "opening";
        case GateState::OPEN:    return "open";
        case GateState::CLOSING: return "closing";
    }
    return "unknown";
}

inline const char* entryStateName(EntryState s) {
    switch (s) {
        case EntryState::IDLE:             return "idle";
        case EntryState::VEHICLE_DETECTED: return "vehicle_detected";
        case EntryState::WAITING_FOR_CARD: return "waiting_for_card";
        case EntryState::ADMITTED:         return "admitted";
        case EntryState::CARD_REJECTED:    return "card_rejected";
        case EntryState::PARKING_FULL:     return "parking_full";
    }
    return "unknown";
}
