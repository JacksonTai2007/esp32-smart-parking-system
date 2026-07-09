#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "AlertService.h"

// =====================================================================
// SafetyService — 环境与安全监测（三路独立，可分别用开关关闭）
//
//  - 火灾报警（ENABLE_FIRE_ALARM）：火焰传感器带确认/解除时长判定，
//    报警时蜂鸣循环警报 + 风扇（继电器）自动启动，火焰消失自动解除
//  - 降雨提示（ENABLE_RAIN_ALERT）：雨滴传感器带确认/解除判定，
//    仅在网页/OLED 提示，不响铃
//  - 碰撞告警（ENABLE_IMPACT_ALERT）：震动传感器沿触发，蜂鸣 1 长响 +
//    网页告警横幅保持数秒自动消失，带冷却时间防刷屏
//
// 设计要点：
//  - 全部输入引脚用内部上/下拉，未接模块时稳定在"未触发"电平，不误报
//  - 全部 millis() 非阻塞；任一开关关闭后对应代码退化为空，仍可编译
// =====================================================================
class SafetyService {
public:
    void begin(AlertService* alerts);
    void update(uint32_t now);

    bool fireAlarm() const { return _fireAlarm; }
    bool rainAlert() const { return _rainAlert; }
    bool impactAlert() const { return _impactAlert; }

private:
    void updateFire(uint32_t now);
    void updateRain(uint32_t now);
    void updateImpact(uint32_t now);
    void fanWrite(bool on);

    AlertService* _alerts = nullptr;

    // 火灾报警
    bool     _fireAlarm     = false;
    bool     _flameRaw      = false;  // 原始火焰状态（去抖前）
    uint32_t _flameSinceMs  = 0;      // 原始状态维持的起始时刻

    // 降雨提示
    bool     _rainAlert     = false;
    bool     _rainRaw       = false;
    uint32_t _rainSinceMs   = 0;

    // 碰撞告警
    bool     _impactAlert   = false;
    bool     _vibRaw        = false;  // 震动原始电平（沿检测用）
    uint32_t _impactAtMs    = 0;      // 最近一次告警触发时刻
};
