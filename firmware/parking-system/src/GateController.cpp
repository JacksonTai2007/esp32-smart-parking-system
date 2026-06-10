#include "GateController.h"

void GateController::begin() {
    // ESP32Servo 需要先分配 LEDC 定时器（官方示例做法）
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    _servo.setPeriodHertz(50);  // SG90 标准 50Hz
    _servo.attach(PIN_GATE_SERVO, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
    _servo.write(GATE_CLOSED_ANGLE);

    _state        = GateState::CLOSED;
    _stateSinceMs = millis();
    LOG_PRINTF("[Gate] init OK, servo on GPIO %u, closed at %d deg\n",
               PIN_GATE_SERVO, GATE_CLOSED_ANGLE);
}

void GateController::setState(GateState s, uint32_t now) {
    _state        = s;
    _stateSinceMs = now;
    LOG_PRINTF("[Gate] -> %s\n", gateStateName(s));
}

void GateController::openGate() {
    const uint32_t now = millis();
    switch (_state) {
        case GateState::OPEN:
            _openedAtMs = now;  // 已开闸：刷新保持计时
            break;
        case GateState::CLOSED:
        case GateState::CLOSING:
            _servo.write(GATE_OPEN_ANGLE);
            setState(GateState::OPENING, now);
            break;
        case GateState::OPENING:
            break;  // 正在开，无需重复动作
    }
}

void GateController::closeGate() {
    const uint32_t now = millis();
    switch (_state) {
        case GateState::OPEN:
        case GateState::OPENING:
            _servo.write(GATE_CLOSED_ANGLE);
            setState(GateState::CLOSING, now);
            break;
        case GateState::CLOSED:
        case GateState::CLOSING:
            break;
    }
}

void GateController::update(uint32_t now) {
    switch (_state) {
        case GateState::OPENING:
            if (now - _stateSinceMs >= GATE_MOTION_TIME_MS) {
                setState(GateState::OPEN, now);
                _openedAtMs = now;
            }
            break;
        case GateState::OPEN:
            if (now - _openedAtMs >= GATE_OPEN_HOLD_MS) {
                LOG_PRINTLN("[Gate] hold time elapsed, auto closing");
                closeGate();
            }
            break;
        case GateState::CLOSING:
            if (now - _stateSinceMs >= GATE_MOTION_TIME_MS) {
                setState(GateState::CLOSED, now);
            }
            break;
        case GateState::CLOSED:
            break;
    }
}
