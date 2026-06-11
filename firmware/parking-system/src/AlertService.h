#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "ParkingTypes.h"

// =====================================================================
// AlertService — 蜂鸣器提示（有源蜂鸣器，电平驱动）
//
// 节奏（全部 millis() 非阻塞）：
//  - SUCCESS：短响 1 次（刷卡成功）
//  - REJECT ：短响 3 次（无效卡）
//  - FULL   ：长响 1 次（车位已满）
//  - ALARM  ：循环间隔响，直到 stopAlarm()（Phase 2 烟雾/火焰预留）
// =====================================================================
class AlertService {
public:
    void begin();
    void update(uint32_t now);

    // 播放一种节奏（会打断当前正在播放的节奏）
    void play(AlertPattern pattern);

    // 停止所有声音（含 ALARM 循环）
    void stopAll();

    bool alarmActive() const { return _active && _pattern == AlertPattern::ALARM; }

private:
    void buzzerWrite(bool on);

    AlertPattern _pattern   = AlertPattern::NONE;
    bool         _active    = false;
    bool         _isOn      = false;
    uint8_t      _beepsLeft = 0;      // 剩余响声次数（ALARM 循环不递减）
    bool         _loop      = false;
    uint32_t     _onMs      = 0;
    uint32_t     _offMs     = 0;
    uint32_t     _nextAtMs  = 0;
};
