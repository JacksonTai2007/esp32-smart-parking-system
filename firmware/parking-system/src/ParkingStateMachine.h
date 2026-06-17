#pragma once
#include <Arduino.h>
#include "../config/Settings.h"
#include "ParkingTypes.h"
#include "GateController.h"
#include "RfidService.h"
#include "UltrasonicService.h"
#include "SlotManager.h"
#include "AlertService.h"

// =====================================================================
// ParkingStateMachine — 主业务状态机
//
// 流程（默认无刷卡，ENABLE_RFID=0）：
//   IDLE --超声波来车--> VEHICLE_DETECTED --短暂停留--> ADMITTED（自动开闸 + 欢迎）
//     满位时 --> PARKING_FULL（蜂鸣 1 长响，不开闸）
//   闸机自动关闭后回到 IDLE；同一辆车需先驶离入口，才会再次放行（去抖防连开）
//
// 启用刷卡（ENABLE_RFID=1）时，在 VEHICLE_DETECTED 之后插入鉴权：
//   WAITING_FOR_CARD --合法卡--> ADMITTED；--非法卡--> CARD_REJECTED
//
// 职责边界：
//  - 输入：UltrasonicService（来车）、RfidService（刷卡）、SlotManager（满位）
//  - 输出：GateController（开关闸）、AlertService（提示音）
//  - OLED / Web 不直接被状态机驱动，它们消费 status() 快照
// =====================================================================
class ParkingStateMachine {
public:
    void begin(GateController* gate, RfidService* rfid, UltrasonicService* sonar,
               SlotManager* slots, AlertService* alerts);
    void update(uint32_t now);

    // 给 OLED / Web 的只读状态快照
    ParkingStatus status() const;

    // Web 手动控制（答辩演示兜底）
    void manualOpenGate();
    void manualCloseGate();
    void resetDemo();

private:
    void enterState(EntryState s, uint32_t now);
    void setMessage(const char* msg);
    void admit(uint32_t now);  // 放行：开闸 + 提示音 + 进入 ADMITTED

    GateController*    _gate   = nullptr;
    RfidService*       _rfid   = nullptr;
    UltrasonicService* _sonar  = nullptr;
    SlotManager*       _slots  = nullptr;
    AlertService*      _alerts = nullptr;

    EntryState _entry          = EntryState::IDLE;
    uint32_t   _entrySinceMs   = 0;
    bool       _vehicleHandled = false;  // 当前在位车辆是否已放行，防止同一辆车反复开闸
    char       _lastUid[RFID_UID_MAX_LEN] = "--";
    char       _message[48]               = "System ready";
};
