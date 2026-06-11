#include "UltrasonicService.h"

void UltrasonicService::begin() {
    pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
    pinMode(PIN_ULTRASONIC_ECHO, INPUT);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    LOG_PRINTF("[Sonar] init OK, trig=%u echo=%u, detect<%.1fcm release>%.1fcm\n",
               PIN_ULTRASONIC_TRIG, PIN_ULTRASONIC_ECHO,
               ULTRASONIC_DETECT_CM, ULTRASONIC_RELEASE_CM);
}

float UltrasonicService::measureOnceCm() {
    // 标准 HC-SR04 时序：10us 触发脉冲，Echo 高电平时长即声波往返时间
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

    // 超时上限 ULTRASONIC_TIMEOUT_US，阻塞最多约 25ms，可接受
    const unsigned long duration = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
    if (duration == 0) {
        return -1.0f;  // 超时：无回波或超出量程
    }
    // 声速 343 m/s -> 0.0343 cm/us，往返除 2
    return duration * 0.0343f / 2.0f;
}

void UltrasonicService::update(uint32_t now) {
    if (now - _lastSampleMs < ULTRASONIC_SAMPLE_INTERVAL_MS) {
        return;
    }
    _lastSampleMs = now;

    const float d = measureOnceCm();
    if (d > 0) {
        _distanceCm = d;
    }

    // 滞回 + 连续确认（计数器饱和即可，不必继续累加）
    if (d > 0 && d < ULTRASONIC_DETECT_CM) {
        if (_nearCount < ULTRASONIC_CONFIRM_SAMPLES) {
            ++_nearCount;
        }
        _farCount = 0;
    } else if (d < 0 || d > ULTRASONIC_RELEASE_CM) {
        if (_farCount < ULTRASONIC_CONFIRM_SAMPLES) {
            ++_farCount;
        }
        _nearCount = 0;
    }
    // DETECT_CM ~ RELEASE_CM 之间为滞回区，维持当前判定不变

    if (!_vehiclePresent && _nearCount >= ULTRASONIC_CONFIRM_SAMPLES) {
        _vehiclePresent = true;
        LOG_PRINTF("[Sonar] vehicle detected at %.1f cm\n", _distanceCm);
    } else if (_vehiclePresent && _farCount >= ULTRASONIC_CONFIRM_SAMPLES) {
        _vehiclePresent = false;
        LOG_PRINTLN("[Sonar] vehicle left");
    }
}
