#include "GateService.h"

#if ENABLE_GATE

// 舵机角度 -> LEDC 占空比。50Hz 周期 20ms，16 位分辨率（满量程 65535 = 20ms）。
// SG90 脉宽约 0.5ms(0°)~2.5ms(180°)，线性映射。
static inline uint32_t degToDuty(uint8_t deg) {
    const uint32_t pulseUs = 500UL + (uint32_t)deg * 2000UL / 180UL;  // 500~2500us
    return pulseUs * 65535UL / 20000UL;
}

void GateService::writeAngle(uint8_t deg) {
    ledcWrite(PIN_GATE_SERVO, degToDuty(deg));
}

void GateService::begin() {
    ledcAttach(PIN_GATE_SERVO, 50, 16);  // 50Hz, 16-bit（ESP32 core 3.x LEDC API）
    _open     = false;
    _openedMs = 0;
    writeAngle(GATE_SERVO_CLOSED_DEG);   // 上电即落杆
    LOG_PRINTF("[Gate] init OK on GPIO %u (closed=%u deg, open=%u deg)\n",
               PIN_GATE_SERVO, GATE_SERVO_CLOSED_DEG, GATE_SERVO_OPEN_DEG);
}

void GateService::open(uint32_t now) {
    _openedMs = now;  // 每次触发都刷新保持计时（连续车流时延长开启）
    if (!_open) {
        _open = true;
        writeAngle(GATE_SERVO_OPEN_DEG);
        LOG_PRINTLN("[Gate] OPEN (raise)");
    }
}

void GateService::close() {
    if (_open) {
        _open = false;
        writeAngle(GATE_SERVO_CLOSED_DEG);
        LOG_PRINTLN("[Gate] CLOSE (lower)");
    }
}

void GateService::update(uint32_t now) {
    if (_open && (now - _openedMs >= GATE_OPEN_HOLD_MS)) {
        close();  // 保持到点自动落杆
    }
}

#else  // !ENABLE_GATE — 空实现，保证关闭功能后仍可编译

void GateService::begin() {
    LOG_PRINTLN("[Gate] disabled by ENABLE_GATE=0");
}
void GateService::update(uint32_t now) { (void)now; }
void GateService::open(uint32_t now)   { (void)now; }
void GateService::close()              {}
void GateService::writeAngle(uint8_t deg) { (void)deg; }

#endif  // ENABLE_GATE
