#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "ParkingTypes.h"

// =====================================================================
// GateController — SG90 舵机闸机控制
//
// 设计：
//  - 默认 GATE_CLOSED_ANGLE(0°) 关闸，GATE_OPEN_ANGLE(90°) 开闸
//  - SG90 无位置反馈，OPENING/CLOSING 按 GATE_MOTION_TIME_MS 计时推进
//  - 开闸后保持 GATE_OPEN_HOLD_MS，到时自动关闸
//  - 全程 millis() 非阻塞，不使用长 delay
// =====================================================================
class GateController {
public:
    void begin();

    // 请求开闸（闸机已开时刷新保持计时，相当于延长开闸时间）
    void openGate();

    // 请求立即关闸
    void closeGate();

    // 每轮主循环调用，推进计时状态
    void update(uint32_t now);

    GateState state() const { return _state; }

private:
    void setState(GateState s, uint32_t now);

    Servo     _servo;
    GateState _state        = GateState::CLOSED;
    uint32_t  _stateSinceMs = 0;  // 进入当前状态的时刻
    uint32_t  _openedAtMs   = 0;  // 完全开闸的时刻，用于自动关闸计时
};
