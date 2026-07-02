#include "ParkingManager.h"

void ParkingManager::begin(SlotManager* slots, AlertService* alerts,
                           BillingService* billing, SafetyService* safety) {
    _slots   = slots;
    _alerts  = alerts;
    _billing = billing;
    _safety  = safety;

    // 以当前车位状态作为初始基线：启动时已被占用的车位从现在开始计时，
    // 不补发入场提示音，也不会凭空生成历史费用。
    const uint32_t now = millis();
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        _occupied[i] = _slots->slotOccupied(i);
        _enterMs[i]  = now;
    }
#if ENABLE_ENTRY_GUIDE
    // 内部上/下拉让未接触摸模块时稳定在"未触发"电平，不会误入场；
    // 触摸感应在模拟模式下同样有效（拍一下 = 网页「车辆到达入口」按钮）
    pinMode(PIN_ENTRY_TOUCH, ENTRY_ACTIVE_LEVEL == HIGH ? INPUT_PULLDOWN : INPUT_PULLUP);
    _entrySinceMs = now;
#endif
    setMessage("System ready");
    LOG_PRINTLN("[PM] parking manager ready");
}

void ParkingManager::setMessage(const char* msg) {
    strlcpy(_message, msg, sizeof(_message));
}

void ParkingManager::update(uint32_t now) {
    updateFireAlarm(now);
    updateEntry(now);

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
#if ENABLE_ENTRY_GUIDE
            // 停入被分配的车位（或任一车位）后，引导任务完成
            if (_assignedSlot == (int8_t)i) {
                _assignedSlot = -1;
            }
#endif
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

// 火警状态由 SafetyService 判定（确认/解除时长、蜂鸣、风扇都在那边）；
// 这里只做业务联动：状态翻转时同步系统消息。
void ParkingManager::updateFireAlarm(uint32_t now) {
    (void)now;
    const bool alarm = (_safety != nullptr) && _safety->fireAlarm();
    if (alarm != _fireAlarmPrev) {
        _fireAlarmPrev = alarm;
        setMessage(alarm ? "FIRE ALARM - fan ON" : "Fire cleared");
    }
}

#if ENABLE_ENTRY_GUIDE
void ParkingManager::triggerEntry(uint32_t now) {
    if (_slots->isFull()) {
        // 满位拒绝入场：长响提示、不分配
        setMessage("Entry denied - Parking FULL");
        _alerts->play(AlertPattern::FULL);
        LOG_PRINTLN("[PM] entry denied: parking full");
        return;
    }
    // 分配编号最小的空闲车位（一次只引导一辆车，重复触发刷新分配）
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        if (!_slots->slotOccupied(i)) {
            _assignedSlot = (int8_t)i;
            _assignedAtMs = now;
            break;
        }
    }
    char msg[48];
    snprintf(msg, sizeof(msg), "Entry: go to P%u", (unsigned)(_assignedSlot + 1));
    setMessage(msg);
    _alerts->play(AlertPattern::ENTER);
    LOG_PRINTF("[PM] entry: assigned P%d\n", _assignedSlot + 1);
}
#endif

// 入口触摸去抖（沿触发）+ 分配超时作废。触摸感应在模拟模式下同样有效；
// 网页 POST /api/entry 经 triggerEntry() 注入等价事件。
void ParkingManager::updateEntry(uint32_t now) {
#if ENABLE_ENTRY_GUIDE
    const bool raw = (digitalRead(PIN_ENTRY_TOUCH) == ENTRY_ACTIVE_LEVEL);
    if (raw != _entryRaw) {
        _entryRaw     = raw;
        _entrySinceMs = now;
    }
    if (raw != _entryStable && now - _entrySinceMs >= ENTRY_DEBOUNCE_MS) {
        _entryStable = raw;
        if (_entryStable) {  // 只在"拍下触摸片"沿触发一次
            triggerEntry(now);
        }
    }
    if (_assignedSlot >= 0 && now - _assignedAtMs >= ENTRY_ASSIGN_TIMEOUT_MS) {
        _assignedSlot = -1;  // 超时未停入，引导作废
        setMessage("Entry guide expired");
    }
#else
    (void)now;
#endif
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

    st.fireAlarm = (_safety != nullptr) && _safety->fireAlarm();
#if ENABLE_ENTRY_GUIDE
    st.assignedSlot = (uint8_t)(_assignedSlot + 1);  // 0 = 无分配
#endif

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

