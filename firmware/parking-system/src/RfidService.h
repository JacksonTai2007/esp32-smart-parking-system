#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "ParkingTypes.h"

#if ENABLE_RFID
#include <SPI.h>
#include <MFRC522.h>
#endif

// =====================================================================
// RfidService — RC522 RFID 读卡与白名单判断
//
// 设计：
//  - 每轮 update() 轮询一次 RC522，读到新卡就生成一个 RfidEvent
//  - 事件由主状态机通过 takeEvent() 一次性取走
//  - 每张新卡的 UID 都打印到串口，方便本地把卡号加入白名单
//  - 同一张卡贴着不动时按 RFID_READ_COOLDOWN_MS 限流，避免事件轰炸
//  - ENABLE_RFID=0 时整个模块退化为空实现，仍可编译
//
// 本模块只做"读取 UID + 比对白名单"，不包含任何破解/复制/绕过功能。
// =====================================================================
class RfidService {
public:
    void begin();
    void update(uint32_t now);

    // 取走待处理的刷卡事件；有事件返回 true 并写入 out
    bool takeEvent(RfidEvent& out);

private:
    bool isWhitelisted(const char* uid) const;

    RfidEvent _pending;
    bool      _hasPending = false;

#if ENABLE_RFID
    MFRC522  _reader{PIN_RFID_SS, PIN_RFID_RST};
    uint32_t _lastReadMs = 0;
    char     _lastUid[RFID_UID_MAX_LEN] = {0};
#endif
};
