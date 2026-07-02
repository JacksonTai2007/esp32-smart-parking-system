#include "SlotManager.h"

void SlotManager::begin() {
    const uint32_t now = millis();
#if ENABLE_SIM_MODE
    // 演示模式：不接红外传感器，车位状态由网页按钮驱动，初始全部空闲。
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        _slots[i].id           = i + 1;
        _simOccupied[i]        = false;
        _slots[i].occupied     = false;
        _slots[i].lastChangeMs = now;
    }
    LOG_PRINTF("[Slots] init OK (SIM mode), %u slots driven by web buttons\n",
               TOTAL_PARKING_SLOTS);
#else
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        // 内部上/下拉把未接线的引脚稳定在"空闲"电平，避免悬空误判"全满"；
        // GPIO 34/35 为输入专用、无内部拉，必须接模块（默认 2 车位不受影响）
        if (PIN_SLOT_SENSOR[i] < 34) {
            pinMode(PIN_SLOT_SENSOR[i],
                    SLOT_OCCUPIED_LEVEL == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
        } else {
            pinMode(PIN_SLOT_SENSOR[i], INPUT);
        }
        _slots[i].id           = i + 1;
        _rawLevel[i]           = digitalRead(PIN_SLOT_SENSOR[i]);
        _rawSinceMs[i]         = now;
        _slots[i].occupied     = (_rawLevel[i] == SLOT_OCCUPIED_LEVEL);
        _slots[i].lastChangeMs = now;
    }
    LOG_PRINTF("[Slots] init OK, %u slots, occupied level=%s, debounce=%lums\n",
               TOTAL_PARKING_SLOTS, SLOT_OCCUPIED_LEVEL == LOW ? "LOW" : "HIGH",
               (unsigned long)SLOT_DEBOUNCE_MS);
#endif
}

void SlotManager::update(uint32_t now) {
#if ENABLE_SIM_MODE
    (void)now;  // 演示模式：车位状态由 Web 按钮驱动，无需轮询引脚
#else
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        const int level = digitalRead(PIN_SLOT_SENSOR[i]);

        // 电平变化时重新计时；保持稳定超过去抖时间才采纳
        if (level != _rawLevel[i]) {
            _rawLevel[i]   = level;
            _rawSinceMs[i] = now;
            continue;
        }
        if (now - _rawSinceMs[i] < SLOT_DEBOUNCE_MS) {
            continue;
        }

        const bool occupied = (level == SLOT_OCCUPIED_LEVEL);
        if (occupied != _slots[i].occupied) {
            _slots[i].occupied     = occupied;
            _slots[i].lastChangeMs = now;
            LOG_PRINTF("[Slots] slot %u -> %s (free %u/%u)\n",
                       _slots[i].id, occupied ? "OCCUPIED" : "FREE",
                       freeSlots(), totalSlots());
        }
    }
#endif
}

uint8_t SlotManager::occupiedSlots() const {
    uint8_t n = 0;
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        if (slotOccupied(i)) {
            ++n;
        }
    }
    return n;
}

bool SlotManager::slotOccupied(uint8_t idx) const {
    if (idx >= TOTAL_PARKING_SLOTS) {
        return false;
    }
#if ENABLE_SIM_MODE
    return _simOccupied[idx];
#else
    return _slots[idx].occupied;
#endif
}

#if ENABLE_SIM_MODE
void SlotManager::simToggle(uint8_t idx) {
    if (idx >= TOTAL_PARKING_SLOTS) {
        return;
    }
    _simOccupied[idx] = !_simOccupied[idx];
    LOG_PRINTF("[Sim] slot %u -> %s (free %u/%u)\n",
               idx + 1, _simOccupied[idx] ? "OCCUPIED" : "FREE",
               freeSlots(), totalSlots());
}
#endif
