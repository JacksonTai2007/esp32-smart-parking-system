#include "ParkingStateMachine.h"

void ParkingStateMachine::begin(GateController* gate, RfidService* rfid,
                                UltrasonicService* sonar, SlotManager* slots,
                                AlertService* alerts) {
    _gate   = gate;
    _rfid   = rfid;
    _sonar  = sonar;
    _slots  = slots;
    _alerts = alerts;

    _entry        = EntryState::IDLE;
    _entrySinceMs = millis();
    setMessage("System ready");
    LOG_PRINTLN("[SM] state machine ready, entry state = idle");
}

void ParkingStateMachine::enterState(EntryState s, uint32_t now) {
    if (_entry == s) {
        return;
    }
    LOG_PRINTF("[SM] %s -> %s\n", entryStateName(_entry), entryStateName(s));
    _entry        = s;
    _entrySinceMs = now;
}

void ParkingStateMachine::setMessage(const char* msg) {
    strlcpy(_message, msg, sizeof(_message));
}

void ParkingStateMachine::update(uint32_t now) {
#if ENABLE_RFID
    // 刷卡事件：记录 UID（方便从网页/串口抄卡号加白名单），
    // 只有 WAITING_FOR_CARD 状态才会用它触发开闸/拒绝业务。
    RfidEvent card;
    const bool hasCard = _rfid->takeEvent(card);
    if (hasCard) {
        strlcpy(_lastUid, card.uid, sizeof(_lastUid));
    }
#endif

    const bool vehicle = _sonar->vehiclePresent();
    const bool full    = _slots->isFull();

    switch (_entry) {
        case EntryState::IDLE:
            if (!vehicle) {
                _vehicleHandled = false;  // 入口空了，重新武装，允许下一辆车
            } else if (!_vehicleHandled) {
                if (full) {
                    enterState(EntryState::PARKING_FULL, now);
                    setMessage("Parking FULL!");
                    _alerts->play(AlertPattern::FULL);
                } else {
                    enterState(EntryState::VEHICLE_DETECTED, now);
                    setMessage("Vehicle detected");
                }
            }
            break;

        case EntryState::VEHICLE_DETECTED:
            if (!vehicle) {
                enterState(EntryState::IDLE, now);
                setMessage("System ready");
            } else if (full) {
                enterState(EntryState::PARKING_FULL, now);
                setMessage("Parking FULL!");
                _alerts->play(AlertPattern::FULL);
            } else if (now - _entrySinceMs >= VEHICLE_DETECTED_DWELL_MS) {
#if ENABLE_RFID
                enterState(EntryState::WAITING_FOR_CARD, now);
                setMessage("Please swipe card");
#else
                admit(now);  // 无刷卡：检测到车辆即自动放行
#endif
            }
            break;

        case EntryState::WAITING_FOR_CARD:
#if ENABLE_RFID
            if (full) {
                enterState(EntryState::PARKING_FULL, now);
                setMessage("Parking FULL!");
                _alerts->play(AlertPattern::FULL);
            } else if (!vehicle) {
                enterState(EntryState::IDLE, now);
                setMessage("System ready");
            } else if (now - _entrySinceMs >= CARD_WAIT_TIMEOUT_MS) {
                enterState(EntryState::IDLE, now);
                setMessage("Card wait timeout");
            } else if (hasCard) {
                if (card.authorized) {
                    admit(now);
                } else {
                    enterState(EntryState::CARD_REJECTED, now);
                    setMessage("Invalid card!");
                    _alerts->play(AlertPattern::REJECT);
                }
            }
#else
            enterState(EntryState::IDLE, now);  // 无刷卡模式不会进入此状态
#endif
            break;

        case EntryState::ADMITTED:
            // 等待整个开闸-保持-关闸周期结束（进入本状态时闸机已在 OPENING，
            // 不会被立即误判为已关闭）；完成后标记本车已放行
            if (_gate->state() == GateState::CLOSED &&
                now - _entrySinceMs >= GATE_MOTION_TIME_MS) {
                _vehicleHandled = true;  // 同一辆车驶离入口前不再重复开闸
                enterState(EntryState::IDLE, now);
                setMessage("System ready");
            }
            break;

        case EntryState::CARD_REJECTED:
#if ENABLE_RFID
            if (now - _entrySinceMs >= REJECT_MESSAGE_MS) {
                if (vehicle && !full) {
                    enterState(EntryState::WAITING_FOR_CARD, now);
                    setMessage("Please swipe card");
                } else if (vehicle && full) {
                    enterState(EntryState::PARKING_FULL, now);
                    setMessage("Parking FULL!");
                    _alerts->play(AlertPattern::FULL);
                } else {
                    enterState(EntryState::IDLE, now);
                    setMessage("System ready");
                }
            }
#else
            enterState(EntryState::IDLE, now);  // 无刷卡模式不会进入此状态
#endif
            break;

        case EntryState::PARKING_FULL:
            if (!vehicle) {
                enterState(EntryState::IDLE, now);
                setMessage("System ready");
            } else if (!full) {
                // 等待中有车位腾出
#if ENABLE_RFID
                enterState(EntryState::WAITING_FOR_CARD, now);
                setMessage("Please swipe card");
#else
                enterState(EntryState::VEHICLE_DETECTED, now);
                setMessage("Vehicle detected");
#endif
            }
            break;
    }
}

void ParkingStateMachine::admit(uint32_t now) {
    enterState(EntryState::ADMITTED, now);
    setMessage("Welcome! Gate opening");
    _alerts->play(AlertPattern::SUCCESS);
    _gate->openGate();
}

ParkingStatus ParkingStateMachine::status() const {
    ParkingStatus st;
    st.totalSlots    = _slots->totalSlots();
    st.occupiedSlots = _slots->occupiedSlots();
    st.freeSlots     = _slots->freeSlots();
    st.gateState     = _gate->state();
    st.entryState    = _entry;
    st.alarmActive   = _alerts->alarmActive();
    st.uptimeMs      = millis();
    strlcpy(st.lastCardUid, _lastUid, sizeof(st.lastCardUid));
    strlcpy(st.lastMessage, _message, sizeof(st.lastMessage));
    for (uint8_t i = 0; i < st.totalSlots && i < MAX_PARKING_SLOTS; ++i) {
        st.slotOccupied[i] = _slots->slotOccupied(i);
    }
    return st;
}

void ParkingStateMachine::manualOpenGate() {
    LOG_PRINTLN("[SM] manual gate OPEN (web)");
    setMessage("Manual open (web)");
    _gate->openGate();
}

void ParkingStateMachine::manualCloseGate() {
    LOG_PRINTLN("[SM] manual gate CLOSE (web)");
    setMessage("Manual close (web)");
    _gate->closeGate();
}

void ParkingStateMachine::resetDemo() {
    LOG_PRINTLN("[SM] demo reset (web)");
    _alerts->stopAll();
    _gate->closeGate();
    _entry          = EntryState::IDLE;
    _entrySinceMs   = millis();
    _vehicleHandled = false;
    strlcpy(_lastUid, "--", sizeof(_lastUid));
    setMessage("Demo reset");
}
