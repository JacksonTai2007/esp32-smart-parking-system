#pragma once
#include <Arduino.h>
#include "../config/Settings.h"
#include "ParkingTypes.h"

// =====================================================================
// BillingService — 停车计费与营收统计（纯软件，不接任何硬件）
//
// 职责：
//  - 按时长计算单次停车费用（规则见 Settings.h 的计费段注释）
//  - 累计总收入与停车次数
//  - 保留最近 MAX_SESSION_LOG 条停车记录（环形覆盖，recent(0) 最新）
//
// 说明："收费"在本项目中仅指本地费用计算与展示，不涉及任何真实支付。
// =====================================================================
class BillingService {
public:
    void begin();

    // 纯函数：根据停留时长计算费用（分）。不改变任何统计。
    uint32_t computeFeeCents(uint32_t durationMs) const;

    // 结算一次停车：计算费用、累加营收与次数、写入最近记录，返回本次费用（分）。
    uint32_t recordSession(uint8_t slotId, uint32_t durationMs);

    // 清零所有统计与记录（Web 重置 / 演示收尾用）
    void reset();

    uint32_t totalRevenueCents() const { return _totalRevenueCents; }
    uint32_t sessionCount()      const { return _sessionCount; }

    uint8_t recentCount() const { return _recentCount; }
    // i = 0 为最近一条；越界返回空记录
    ParkingSession recent(uint8_t i) const;

private:
    uint32_t       _totalRevenueCents = 0;
    uint32_t       _sessionCount      = 0;
    ParkingSession _log[MAX_SESSION_LOG];
    uint8_t        _recentCount       = 0;  // 已填充条数（<= MAX_SESSION_LOG）
    uint8_t        _head              = 0;  // 下一个写入位置（环形）
};
