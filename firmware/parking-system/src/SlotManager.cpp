#include "SlotManager.h"

void SlotManager::begin() {
    const uint32_t now = millis();
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        pinMode(PIN_SLOT_SENSOR[i], INPUT);
        _slots[i].id           = i + 1;
        _rawLevel[i]           = digitalRead(PIN_SLOT_SENSOR[i]);
        _rawSinceMs[i]         = now;
        _slots[i].occupied     = (_rawLevel[i] == SLOT_OCCUPIED_LEVEL);
        _slots[i].lastChangeMs = now;
    }
    LOG_PRINTF("[Slots] init OK, %u slots, occupied level=%s, debounce=%lums\n",
               TOTAL_PARKING_SLOTS, SLOT_OCCUPIED_LEVEL == LOW ? "LOW" : "HIGH",
               (unsigned long)SLOT_DEBOUNCE_MS);
}

void SlotManager::update(uint32_t now) {
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
}

uint8_t SlotManager::occupiedSlots() const {
    uint8_t n = 0;
    for (uint8_t i = 0; i < TOTAL_PARKING_SLOTS; ++i) {
        if (_slots[i].occupied) {
            ++n;
        }
    }
    return n;
}

bool SlotManager::slotOccupied(uint8_t idx) const {
    if (idx >= TOTAL_PARKING_SLOTS) {
        return false;
    }
    return _slots[idx].occupied;
}
