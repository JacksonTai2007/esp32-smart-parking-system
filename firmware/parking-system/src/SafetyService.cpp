#include "SafetyService.h"

#if ENABLE_FIRE_ALARM

void SafetyService::begin(AlertService* alerts) {
    _alerts = alerts;

    // 未接传感器时靠内部上/下拉稳定在"无火焰"电平，避免浮空误报警
    pinMode(PIN_FLAME_SENSOR, FLAME_ACTIVE_LEVEL == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    pinMode(PIN_FAN_CONTROL, OUTPUT);
    fanWrite(false);

    _alarm        = false;
    _flameRaw     = false;
    _flameSinceMs = millis();
    LOG_PRINTF("[Safety] init OK (flame GPIO %u, fan GPIO %u)\n",
               PIN_FLAME_SENSOR, PIN_FAN_CONTROL);
}

void SafetyService::fanWrite(bool on) {
    digitalWrite(PIN_FAN_CONTROL, on ? FAN_ACTIVE_LEVEL : !FAN_ACTIVE_LEVEL);
}

void SafetyService::update(uint32_t now) {
    const bool flame = (digitalRead(PIN_FLAME_SENSOR) == FLAME_ACTIVE_LEVEL);
    if (flame != _flameRaw) {
        _flameRaw     = flame;
        _flameSinceMs = now;  // 原始状态翻转，重新计稳定时长
    }
    const uint32_t stableMs = now - _flameSinceMs;

    if (!_alarm && flame && stableMs >= FIRE_CONFIRM_MS) {
        _alarm = true;
        fanWrite(true);
        _alerts->play(AlertPattern::ALARM);
        LOG_PRINTLN("[Safety] FIRE ALARM raised - fan ON");
    } else if (_alarm && !flame && stableMs >= FIRE_CLEAR_MS) {
        _alarm = false;
        fanWrite(false);
        if (_alerts->alarmActive()) {
            _alerts->stopAll();
        }
        LOG_PRINTLN("[Safety] fire cleared - fan OFF");
    }

    // 报警期间若被进出场提示音打断（play 会抢占节奏），提示音结束后恢复循环警报
    if (_alarm && !_alerts->alarmActive()) {
        _alerts->play(AlertPattern::ALARM);
    }
}

#else  // !ENABLE_FIRE_ALARM — 空实现，保证关闭功能后仍可编译

void SafetyService::begin(AlertService* alerts) {
    _alerts = alerts;
    LOG_PRINTLN("[Safety] disabled by ENABLE_FIRE_ALARM=0");
}
void SafetyService::update(uint32_t now) { (void)now; }
void SafetyService::fanWrite(bool on)    { (void)on; }

#endif  // ENABLE_FIRE_ALARM
