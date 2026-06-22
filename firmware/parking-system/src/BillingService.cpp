#include "BillingService.h"

void BillingService::begin() {
    reset();
    LOG_PRINTF("[Bill] init OK, free %lus then %lu cents/min\n",
               (unsigned long)PARKING_FREE_PERIOD_SEC,
               (unsigned long)PARKING_RATE_PER_MIN_CENTS);
}

uint32_t BillingService::computeFeeCents(uint32_t durationMs) const {
    // 免费时长内不计费（短暂停靠 / 误触发不收费）
    if (durationMs <= PARKING_FREE_PERIOD_SEC * 1000UL) {
        return 0;
    }
    // 向上取整到分钟：ceil(durationMs / 60000)
    const uint32_t minutes = (durationMs + 60000UL - 1) / 60000UL;
    return minutes * PARKING_RATE_PER_MIN_CENTS;
}

uint32_t BillingService::recordSession(uint8_t slotId, uint32_t durationMs) {
    const uint32_t fee = computeFeeCents(durationMs);

    _totalRevenueCents += fee;
    ++_sessionCount;

    _log[_head].slotId     = slotId;
    _log[_head].durationMs = durationMs;
    _log[_head].feeCents   = fee;
    _head = (_head + 1) % MAX_SESSION_LOG;
    if (_recentCount < MAX_SESSION_LOG) {
        ++_recentCount;
    }

    LOG_PRINTF("[Bill] slot %u parked %lums -> fee %lu cents (total %lu, count %lu)\n",
               slotId, (unsigned long)durationMs, (unsigned long)fee,
               (unsigned long)_totalRevenueCents, (unsigned long)_sessionCount);
    return fee;
}

void BillingService::reset() {
    _totalRevenueCents = 0;
    _sessionCount      = 0;
    _recentCount       = 0;
    _head              = 0;
    for (uint8_t i = 0; i < MAX_SESSION_LOG; ++i) {
        _log[i] = ParkingSession{};
    }
}

ParkingSession BillingService::recent(uint8_t i) const {
    if (i >= _recentCount) {
        return ParkingSession{};
    }
    // _head 指向下一个写入位（即最旧记录之后）；最近一条在 _head-1。
    const uint8_t idx = (_head + MAX_SESSION_LOG - 1 - i) % MAX_SESSION_LOG;
    return _log[idx];
}
