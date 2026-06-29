// =====================================================================
// parking-system.ino — 基于 ESP32 的智能停车场管理系统（简化版）
//
// 本期方案：红外车位识别 + 车位管理 + 按时长计费（无 RFID、无闸机）。
//
// 主程序只负责初始化与调度，业务逻辑全部在 src/ 各模块中：
//   SlotManager     红外车位传感器 + 占用判定（去抖）
//   ParkingManager  主业务：识别车辆进出、触发计费、生成状态快照
//   BillingService  按时长计费 + 营收/记录统计
//   AlertService    蜂鸣器提示
//   GateService     出入口道闸（SG90 舵机，进出场自动抬杆 + 手动开/落）
//   DisplayService  OLED SSD1306 显示
//   WebDashboard    Wi-Fi + 网页 + JSON API
//
// 引脚见 config/Pins.h，参数与功能开关见 config/Settings.h。
// 主循环为 millis() 非阻塞调度，禁止长时间 delay()。
// =====================================================================

#include "config/Pins.h"
#include "config/Settings.h"
#include "src/ParkingTypes.h"
#include "src/SlotManager.h"
#include "src/AlertService.h"
#include "src/BillingService.h"
#include "src/GateService.h"
#include "src/DisplayService.h"
#include "src/ParkingManager.h"
#include "src/WebDashboard.h"

SlotManager     slotManager;
AlertService    alertService;
BillingService  billingService;
GateService     gateService;
DisplayService  displayService;
ParkingManager  parkingManager;
WebDashboard    webDashboard;

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);  // 仅启动时等待串口稳定，主循环中禁止此类 delay

    Serial.println();
    Serial.println("=============================================");
    Serial.println("  ESP32 Smart Parking System (slots + billing)");
    Serial.println("=============================================");
    Serial.printf("Features: OLED=%d WEB=%d BUZZER=%d DEBUG=%d\n",
                  ENABLE_DISPLAY, ENABLE_WEB, ENABLE_BUZZER, ENABLE_DEBUG_SERIAL);
    Serial.printf("Slots: %u, free %lus then %lu cents/min\n",
                  TOTAL_PARKING_SLOTS, (unsigned long)PARKING_FREE_PERIOD_SEC,
                  (unsigned long)PARKING_RATE_PER_MIN_CENTS);

    // 各模块初始化（每个 begin 自行打印状态）
    alertService.begin();
    displayService.begin();
    slotManager.begin();
    billingService.begin();
    gateService.begin();
    parkingManager.begin(&slotManager, &alertService, &billingService, &gateService);
    webDashboard.begin(&parkingManager);

    Serial.println("[Boot] setup complete, entering main loop");
}

void loop() {
    const uint32_t now = millis();

    // 输入采集
    slotManager.update(now);

    // 业务决策（识别车辆进出 + 触发计费）
    parkingManager.update(now);

    // 执行器与输出
    alertService.update(now);
    gateService.update(now);  // 抬杆保持到点自动落杆
    displayService.update(now, parkingManager.status(), webDashboard.networkSummary());
    webDashboard.update(now);
}
