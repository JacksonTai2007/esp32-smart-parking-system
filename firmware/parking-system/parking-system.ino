// =====================================================================
// parking-system.ino — 基于 ESP32 的智能停车场管理系统（MVP）
//
// 主程序只负责初始化与调度，业务逻辑全部在 src/ 各模块中：
//   ParkingStateMachine  主业务状态机
//   GateController       SG90 舵机闸机
//   RfidService          RC522 刷卡 + 白名单
//   UltrasonicService    HC-SR04 入口来车检测
//   SlotManager          红外车位传感器 + 剩余车位统计
//   DisplayService       OLED SSD1306 显示
//   AlertService         蜂鸣器提示
//   WebDashboard         Wi-Fi + 网页 + JSON API
//
// 引脚见 config/Pins.h，参数与功能开关见 config/Settings.h。
// 主循环为 millis() 非阻塞调度，禁止长时间 delay()。
// =====================================================================

#include "config/Pins.h"
#include "config/Settings.h"
#include "src/ParkingTypes.h"
#include "src/GateController.h"
#include "src/RfidService.h"
#include "src/UltrasonicService.h"
#include "src/SlotManager.h"
#include "src/AlertService.h"
#include "src/DisplayService.h"
#include "src/ParkingStateMachine.h"
#include "src/WebDashboard.h"

GateController      gateController;
RfidService         rfidService;
UltrasonicService   ultrasonicService;
SlotManager         slotManager;
AlertService        alertService;
DisplayService      displayService;
ParkingStateMachine parkingSm;
WebDashboard        webDashboard;

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);  // 仅启动时等待串口稳定，主循环中禁止此类 delay

    Serial.println();
    Serial.println("=============================================");
    Serial.println("  ESP32 Smart Parking System (MVP, Phase 1)");
    Serial.println("=============================================");
    Serial.printf("Features: RFID=%d OLED=%d WEB=%d BUZZER=%d DEBUG=%d\n",
                  ENABLE_RFID, ENABLE_DISPLAY, ENABLE_WEB, ENABLE_BUZZER,
                  ENABLE_DEBUG_SERIAL);
    Serial.printf("Slots: %u, gate %d->%d deg, hold %lums\n",
                  TOTAL_PARKING_SLOTS, GATE_CLOSED_ANGLE, GATE_OPEN_ANGLE,
                  (unsigned long)GATE_OPEN_HOLD_MS);

    // 各模块初始化（每个 begin 自行打印状态）
    alertService.begin();
    displayService.begin();
    gateController.begin();
    ultrasonicService.begin();
    slotManager.begin();
    rfidService.begin();
    parkingSm.begin(&gateController, &rfidService, &ultrasonicService,
                    &slotManager, &alertService);
    webDashboard.begin(&parkingSm);

    Serial.println("[Boot] setup complete, entering main loop");
}

void loop() {
    const uint32_t now = millis();

    // 输入采集
    ultrasonicService.update(now);
    rfidService.update(now);
    slotManager.update(now);

    // 业务决策
    parkingSm.update(now);

    // 执行器与输出
    gateController.update(now);
    alertService.update(now);
    displayService.update(now, parkingSm.status(), webDashboard.networkSummary());
    webDashboard.update(now);
}
