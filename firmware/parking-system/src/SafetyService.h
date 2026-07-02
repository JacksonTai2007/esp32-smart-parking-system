#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "AlertService.h"

// =====================================================================
// SafetyService — 火灾报警（火焰传感器）+ 风扇/继电器联动
//
// 设计：
//  - 火焰传感器为数字输入（检测到火焰输出 FLAME_ACTIVE_LEVEL）；
//    引脚用内部上/下拉，未接线时稳定在"无火焰"电平，不会误报警
//  - 判定带确认时间：连续检测到火焰 FIRE_CONFIRM_MS 才报警（防闪烁误报），
//    火焰消失 FIRE_CLEAR_MS 才解除（防临界抖动反复触发）
//  - 报警期间：蜂鸣器 ALARM 循环响 + 风扇（继电器）自动启动；
//    解除后自动停响、停风扇。全部 millis() 非阻塞
//  - ENABLE_FIRE_ALARM=0 时退化为空实现，固件照常编译
// =====================================================================
class SafetyService {
public:
    void begin(AlertService* alerts);
    void update(uint32_t now);
    bool fireAlarm() const { return _alarm; }

private:
    void fanWrite(bool on);

    AlertService* _alerts       = nullptr;
    bool          _alarm        = false;
    bool          _flameRaw     = false;  // 上一轮读到的原始火焰状态
    uint32_t      _flameSinceMs = 0;      // 原始状态维持的起始时刻
};
