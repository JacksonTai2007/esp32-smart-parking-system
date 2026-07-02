#pragma once
#include <Arduino.h>
#include "../config/Settings.h"
#include "ParkingTypes.h"
#include "SlotManager.h"
#include "AlertService.h"
#include "BillingService.h"
#include "SafetyService.h"

// =====================================================================
// ParkingManager — 主业务逻辑（车辆识别 + 车位管理 + 计费触发）
//
// 流程（无 RFID、无闸机，全部由车位红外传感器驱动）：
//   车位 空闲 -> 占用：识别为车辆入场，开始计时（蜂鸣 1 短响）
//   车位 占用 -> 空闲：车辆离场，按停留时长结算费用（蜂鸣 2 短响）
//   占满全部车位：提示满位（蜂鸣 1 长响）
//
// 职责边界：
//  - 输入：SlotManager（车位占用状态 + 去抖）
//  - 输出：BillingService（计费/营收）、AlertService（提示音）
//  - OLED / Web 不直接被驱动，它们消费 status() 快照
// =====================================================================
class ParkingManager {
public:
    void begin(SlotManager* slots, AlertService* alerts, BillingService* billing,
               SafetyService* safety);
    void update(uint32_t now);

    // 给 OLED / Web 的只读状态快照
    ParkingStatus status() const;

    // Web 管理：清零累计收入与最近记录（演示收尾兜底）
    void resetStats();

#if ENABLE_SIM_MODE
    // 演示模式：网页按钮把"模拟车辆驶入/驶离"转发给 SlotManager，
    // 下一轮 update() 会按真实进出流程识别并计费。
    void simToggleSlot(uint8_t idx);
#endif

#if ENABLE_ENTRY_GUIDE
    // 智能入场事件：入口触摸感应触发或网页 POST /api/entry。
    // 有空位 -> 自动分配编号最小的空闲车位并引导（横幅/OLED/蜂鸣）；
    // 满位   -> 拒绝入场（长响提示）。
    void triggerEntry(uint32_t now);
#endif

private:
    void setMessage(const char* msg);
    void updateFireAlarm(uint32_t now);   // 火警状态变化 -> 消息同步
    void updateEntry(uint32_t now);       // 入口触摸去抖 + 分配超时作废

    SlotManager*    _slots   = nullptr;
    AlertService*   _alerts  = nullptr;
    BillingService* _billing = nullptr;
    SafetyService*  _safety  = nullptr;

    bool     _occupied[MAX_PARKING_SLOTS] = {false};  // 本管理器跟踪的车位状态
    uint32_t _enterMs[MAX_PARKING_SLOTS]  = {0};       // 车辆入场时刻（占用起算）
    char     _message[48]                 = "System ready";
    bool     _fireAlarmPrev               = false;     // 上一轮的火警状态（沿检测用）
#if ENABLE_ENTRY_GUIDE
    int8_t   _assignedSlot  = -1;     // 已分配待停入的车位下标，-1 = 无
    uint32_t _assignedAtMs  = 0;      // 分配时刻（超时作废用）
    bool     _entryRaw      = false;  // 入口触摸原始电平（去抖前）
    bool     _entryStable   = false;  // 去抖后的稳定状态
    uint32_t _entrySinceMs  = 0;      // 原始状态维持的起始时刻
#endif
};
