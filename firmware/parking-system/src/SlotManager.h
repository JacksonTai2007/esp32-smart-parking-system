#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "ParkingTypes.h"

// =====================================================================
// SlotManager — 车位状态与剩余车位统计
//
// 设计：
//  - 支持 2~4 个红外车位传感器（数量由 TOTAL_PARKING_SLOTS 配置）
//  - 触发电平由 SLOT_OCCUPIED_LEVEL 配置（常见红外模块：有车输出 LOW）
//  - SLOT_DEBOUNCE_MS 软件去抖：原始电平稳定一段时间后才更新车位状态
//  - GPIO 34/35 是输入专用脚且无内部上拉，红外模块自带推挽输出，可直接接
// =====================================================================
class SlotManager {
public:
    void begin();
    void update(uint32_t now);

    uint8_t totalSlots() const { return TOTAL_PARKING_SLOTS; }
    uint8_t occupiedSlots() const;
    uint8_t freeSlots() const { return TOTAL_PARKING_SLOTS - occupiedSlots(); }
    bool    isFull() const { return occupiedSlots() >= TOTAL_PARKING_SLOTS; }

    // idx 为 0..TOTAL_PARKING_SLOTS-1，越界返回 false
    bool slotOccupied(uint8_t idx) const;

private:
    ParkingSlot _slots[MAX_PARKING_SLOTS];
    int         _rawLevel[MAX_PARKING_SLOTS]   = {0};  // 最近一次原始电平
    uint32_t    _rawSinceMs[MAX_PARKING_SLOTS] = {0};  // 原始电平保持起始时刻
};
