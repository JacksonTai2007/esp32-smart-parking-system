#include "AlertService.h"

void AlertService::buzzerWrite(bool on) {
#if ENABLE_BUZZER
    digitalWrite(PIN_BUZZER, on ? BUZZER_ACTIVE_LEVEL : !BUZZER_ACTIVE_LEVEL);
#else
    (void)on;
#endif
    _isOn = on;
}

void AlertService::begin() {
#if ENABLE_BUZZER
    pinMode(PIN_BUZZER, OUTPUT);
    buzzerWrite(false);
    LOG_PRINTF("[Alert] init OK, buzzer on GPIO %u, active level=%s\n",
               PIN_BUZZER, BUZZER_ACTIVE_LEVEL == HIGH ? "HIGH" : "LOW");
#else
    LOG_PRINTLN("[Alert] disabled by ENABLE_BUZZER=0");
#endif
}

void AlertService::play(AlertPattern pattern) {
#if !ENABLE_BUZZER
    (void)pattern;
    return;
#else
    _pattern = pattern;
    _loop    = false;

    switch (pattern) {
        case AlertPattern::ENTER:
            _beepsLeft = 1;
            _onMs      = BEEP_SHORT_MS;
            _offMs     = BEEP_GAP_MS;
            break;
        case AlertPattern::EXIT:
            _beepsLeft = 2;
            _onMs      = BEEP_SHORT_MS;
            _offMs     = BEEP_GAP_MS;
            break;
        case AlertPattern::FULL:
            _beepsLeft = 1;
            _onMs      = BEEP_LONG_MS;
            _offMs     = BEEP_GAP_MS;
            break;
        case AlertPattern::ALARM:
            _beepsLeft = 1;
            _loop      = true;
            _onMs      = ALARM_INTERVAL_MS;
            _offMs     = ALARM_INTERVAL_MS;
            break;
        case AlertPattern::NONE:
        default:
            stopAll();
            return;
    }

    // 立即进入第一段"响"
    _active   = true;
    _nextAtMs = millis() + _onMs;
    buzzerWrite(true);
#endif
}

void AlertService::stopAll() {
    _active    = false;
    _loop      = false;
    _beepsLeft = 0;
    _pattern   = AlertPattern::NONE;
    buzzerWrite(false);
}

void AlertService::update(uint32_t now) {
    if (!_active || (int32_t)(now - _nextAtMs) < 0) {
        return;
    }

    if (_isOn) {
        // "响"结束 -> 进入"停"
        buzzerWrite(false);
        if (!_loop) {
            --_beepsLeft;
        }
        if (_beepsLeft == 0 && !_loop) {
            _active = false;
            return;
        }
        _nextAtMs = now + _offMs;
    } else {
        // "停"结束 -> 下一段"响"
        buzzerWrite(true);
        _nextAtMs = now + _onMs;
    }
}
