#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"

// =====================================================================
// UltrasonicService — HC-SR04 入口车辆靠近检测
//
// 设计：
//  - 每 ULTRASONIC_SAMPLE_INTERVAL_MS 采样一次，不在主循环里连续测距
//  - pulseIn 设置 ULTRASONIC_TIMEOUT_US 超时（约 4.3 m），避免长阻塞
//  - 来车判定带滞回：< DETECT_CM 连续 N 次确认"有车"，
//    > RELEASE_CM 连续 N 次确认"车辆离开"，抗抖动
//  - 阈值全部来自 Settings.h，本地实测时按实际场景校准
// =====================================================================
class UltrasonicService {
public:
    void begin();
    void update(uint32_t now);

    // 最近一次有效距离（cm）；尚无有效读数时返回 -1
    float distanceCm() const { return _distanceCm; }

    // 滞回滤波后的"入口有车"判定
    bool vehiclePresent() const { return _vehiclePresent; }

private:
    float measureOnceCm();

    float    _distanceCm     = -1.0f;
    bool     _vehiclePresent = false;
    uint32_t _lastSampleMs   = 0;
    uint8_t  _nearCount      = 0;  // 连续"近距离"样本数
    uint8_t  _farCount       = 0;  // 连续"远距离/无效"样本数
};
