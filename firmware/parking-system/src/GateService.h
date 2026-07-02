#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"

// =====================================================================
// GateService — 出入口道闸（SG90 舵机）
//
// 设计：
//  - 用 ESP32 自带 LEDC 生成 50Hz PWM 直接驱动舵机，不依赖第三方库
//  - 非阻塞：open() 立即抬杆并记录时刻，update() 到点（GATE_OPEN_HOLD_MS）
//    自动落杆；close() 立即落杆。主循环无 delay()
//  - 角度由 Settings.h 的 GATE_SERVO_CLOSED_DEG / GATE_SERVO_OPEN_DEG 配置
//  - ENABLE_GATE=0 时全部退化为空实现，固件照常编译
// =====================================================================
class GateService {
public:
    void begin();
    void update(uint32_t now);
    void open(uint32_t now);   // 抬杆放行，并开始保持计时
    void close();              // 立即落杆挡车
    bool isOpen() const { return _open; }

private:
    void writeAngle(uint8_t deg);

    bool     _open     = false;
    uint32_t _openedMs = 0;
};
