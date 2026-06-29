#include "ParkingManager.h"

void ParkingManager::begin(SlotManager* slots, AlertService* alerts,
                           BillingService* billing) {
    _slots   = slots;
    _alerts  = alerts;
    _billing = billing;

    // 以当前车位状态作为初始基线：启动时已被占用的车位从现在开始计时，
    // 不补发入场提示音，也不会凭空生成历史费用。
    const uint32_t now = millis();
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        _occupied[i] = _slots->slotOccupied(i);
        _enterMs[i]  = now;
    }
    setMessage("System ready");
    LOG_PRINTLN("[PM] parking manager ready");
}

void ParkingManager::setMessage(const char* msg) {
    strlcpy(_message, msg, sizeof(_message));
}

void ParkingManager::update(uint32_t now) {
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        const bool occupied = _slots->slotOccupied(i);
        if (occupied == _occupied[i]) {
            continue;  // 状态未变
        }
        _occupied[i] = occupied;
        const uint8_t slotId = i + 1;

        if (occupied) {
            // 车辆入场：开始计时
            _enterMs[i] = now;
            if (_slots->isFull()) {
                char msg[48];
                snprintf(msg, sizeof(msg), "P%u in - Parking FULL", slotId);
                setMessage(msg);
                _alerts->play(AlertPattern::FULL);
            } else {
                char msg[48];
                snprintf(msg, sizeof(msg), "P%u in - timing started", slotId);
                setMessage(msg);
                _alerts->play(AlertPattern::ENTER);
            }
        } else {
            // 车辆离场：结算费用
            const uint32_t durationMs = now - _enterMs[i];
            const uint32_t feeCents   = _billing->recordSession(slotId, durationMs);

            char durBuf[12], feeBuf[12], msg[48];
            formatDurationMs(durBuf, sizeof(durBuf), durationMs);
            formatMoneyCents(feeBuf, sizeof(feeBuf), feeCents);
            // 消息同时显示在 OLED 上，保持纯 ASCII（货币符号只在网页拼接）
            snprintf(msg, sizeof(msg), "P%u left %s  %s", slotId, durBuf, feeBuf);
            setMessage(msg);
            _alerts->play(AlertPattern::EXIT);
        }
    }
}

ParkingStatus ParkingManager::status() const {
    const uint32_t now = millis();

    ParkingStatus st;
    st.totalSlots    = _slots->totalSlots();
    st.occupiedSlots = _slots->occupiedSlots();
    st.freeSlots     = _slots->freeSlots();
    for (uint8_t i = 0; i < st.totalSlots && i < MAX_PARKING_SLOTS; ++i) {
        const bool occ = _slots->slotOccupied(i);
        st.slotOccupied[i]   = occ;
        st.slotDurationMs[i] = occ ? (now - _enterMs[i]) : 0;
    }

    st.totalRevenueCents = _billing->totalRevenueCents();
    st.sessionCount      = _billing->sessionCount();
    st.recentCount       = _billing->recentCount();
    for (uint8_t i = 0; i < st.recentCount && i < MAX_SESSION_LOG; ++i) {
        st.recent[i] = _billing->recent(i);
    }

    strlcpy(st.lastMessage, _message, sizeof(st.lastMessage));
    st.uptimeMs = now;
    return st;
}

void ParkingManager::resetStats() {
    LOG_PRINTLN("[PM] stats reset (web)");
    _billing->reset();
    // 顺便把在场车辆的入场计时也归零：点"清零"相当于本轮演示重新开始，
    // 此后这些车离场时按"从重置时刻起"的时长计费，而非原始入场时刻。
    const uint32_t now = millis();
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        _enterMs[i] = now;
    }
    setMessage("Stats reset");
}

#if ENABLE_SIM_MODE
void ParkingManager::simToggleSlot(uint8_t idx) {
    if (_slots) {
        _slots->simToggle(idx);
    }
}
#endif
