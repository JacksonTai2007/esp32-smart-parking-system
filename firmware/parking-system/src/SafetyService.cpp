#include "SafetyService.h"

void SafetyService::begin(AlertService* alerts) {
    _alerts = alerts;
    const uint32_t now = millis();

#if ENABLE_FIRE_ALARM
    // 未接传感器时靠内部上/下拉稳定在"无火焰"电平，避免浮空误报警
    pinMode(PIN_FLAME_SENSOR, FLAME_ACTIVE_LEVEL == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    pinMode(PIN_FAN_CONTROL, OUTPUT);
    fanWrite(false);
    _flameSinceMs = now;
    LOG_PRINTF("[Safety] fire alarm ON (flame GPIO %u, fan GPIO %u)\n",
               PIN_FLAME_SENSOR, PIN_FAN_CONTROL);
#else
    LOG_PRINTLN("[Safety] fire alarm disabled by ENABLE_FIRE_ALARM=0");
#endif

#if ENABLE_RAIN_ALERT
    pinMode(PIN_RAIN_SENSOR, RAIN_ACTIVE_LEVEL == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    _rainSinceMs = now;
    LOG_PRINTF("[Safety] rain alert ON (GPIO %u)\n", PIN_RAIN_SENSOR);
#endif

#if ENABLE_IMPACT_ALERT
    pinMode(PIN_VIBRATION_SENSOR,
            IMPACT_ACTIVE_LEVEL == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    LOG_PRINTF("[Safety] impact alert ON (GPIO %u)\n", PIN_VIBRATION_SENSOR);
#endif
}

void SafetyService::fanWrite(bool on) {
#if ENABLE_FIRE_ALARM
    digitalWrite(PIN_FAN_CONTROL, on ? FAN_ACTIVE_LEVEL : !FAN_ACTIVE_LEVEL);
#else
    (void)on;
#endif
}

void SafetyService::update(uint32_t now) {
    updateFire(now);
    updateRain(now);
    updateImpact(now);
}

void SafetyService::updateFire(uint32_t now) {
#if ENABLE_FIRE_ALARM
    const bool flame = (digitalRead(PIN_FLAME_SENSOR) == FLAME_ACTIVE_LEVEL);
    if (flame != _flameRaw) {
        _flameRaw     = flame;
        _flameSinceMs = now;  // 原始状态翻转，重新计稳定时长
    }
    const uint32_t stableMs = now - _flameSinceMs;

    if (!_fireAlarm && flame && stableMs >= FIRE_CONFIRM_MS) {
        _fireAlarm = true;
        fanWrite(true);
        _alerts->play(AlertPattern::ALARM);
        LOG_PRINTLN("[Safety] FIRE ALARM raised - fan ON");
    } else if (_fireAlarm && !flame && stableMs >= FIRE_CLEAR_MS) {
        _fireAlarm = false;
        fanWrite(false);
        if (_alerts->alarmActive()) {
            _alerts->stopAll();
        }
        LOG_PRINTLN("[Safety] fire cleared - fan OFF");
    }

    // 报警期间若被进出场提示音打断（play 会抢占节奏），提示音结束后恢复循环警报
    if (_fireAlarm && !_alerts->alarmActive()) {
        _alerts->play(AlertPattern::ALARM);
    }
#else
    (void)now;
#endif
}

void SafetyService::updateRain(uint32_t now) {
#if ENABLE_RAIN_ALERT
    const bool rain = (digitalRead(PIN_RAIN_SENSOR) == RAIN_ACTIVE_LEVEL);
    if (rain != _rainRaw) {
        _rainRaw     = rain;
        _rainSinceMs = now;
    }
    const uint32_t stableMs = now - _rainSinceMs;

    if (!_rainAlert && rain && stableMs >= RAIN_CONFIRM_MS) {
        _rainAlert = true;  // 纯提示，不响铃
        LOG_PRINTLN("[Safety] rain detected");
    } else if (_rainAlert && !rain && stableMs >= RAIN_CLEAR_MS) {
        _rainAlert = false;
        LOG_PRINTLN("[Safety] rain cleared");
    }
#else
    (void)now;
#endif
}

void SafetyService::updateImpact(uint32_t now) {
#if ENABLE_IMPACT_ALERT
    const bool vib = (digitalRead(PIN_VIBRATION_SENSOR) == IMPACT_ACTIVE_LEVEL);
    // 震动模块输出脉冲，做"沿触发 + 冷却时间"，不做常规去抖
    if (vib && !_vibRaw && now - _impactAtMs >= IMPACT_COOLDOWN_MS) {
        _impactAtMs  = now;
        _impactAlert = true;
        if (!_fireAlarm) {  // 火警循环优先，不被单次撞击音打断
            _alerts->play(AlertPattern::FULL);  // 1 长响示警
        }
        LOG_PRINTLN("[Safety] IMPACT detected");
    }
    _vibRaw = vib;

    if (_impactAlert && now - _impactAtMs >= IMPACT_ALERT_HOLD_MS) {
        _impactAlert = false;  // 告警横幅保持到点自动消失
    }
#else
    (void)now;
#endif
}
