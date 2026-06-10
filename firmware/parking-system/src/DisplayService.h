#pragma once
#include <Arduino.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "ParkingTypes.h"

#if ENABLE_DISPLAY
#include <Adafruit_SSD1306.h>
#endif

// =====================================================================
// DisplayService — OLED SSD1306 (I2C) 状态显示
//
// 设计：
//  - ENABLE_DISPLAY=0 时不引用 Adafruit 库，模块退化为空实现仍可编译
//  - 初始化失败（地址不对/没接屏）只警告，不影响系统其余功能
//  - 每 DISPLAY_REFRESH_MS 重绘一次，由主循环驱动，非阻塞
//
// 128x64 字号 1 共 8 行，布局：
//   Smart Parking
//   ----------------
//   Free 3/4
//   1:_ 2:X 3:_ 4:_      (车位地图, X=占用 _=空闲)
//   Gate: open
//   Card: A1:B2:C3:D4
//   <最近消息>
//   <网络状态/IP>
// =====================================================================
class DisplayService {
public:
    // 返回是否初始化成功；失败时后续 update 自动跳过
    bool begin();

    // netInfo 由 WebDashboard 提供，如 "IP 192.168.1.5" / "AP 192.168.4.1"
    void update(uint32_t now, const ParkingStatus& st, const char* netInfo);

private:
    void splash();

    bool     _ok         = false;
    uint32_t _lastDrawMs = 0;
#if ENABLE_DISPLAY
    Adafruit_SSD1306* _oled = nullptr;
#endif
};
