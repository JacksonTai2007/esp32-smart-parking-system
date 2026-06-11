#include "DisplayService.h"

#if ENABLE_DISPLAY

#include <Wire.h>
#include <Adafruit_GFX.h>

bool DisplayService::begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    _oled = new Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

    if (!_oled->begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
        LOG_PRINTF("[OLED] init FAILED at 0x%02X (检查接线/地址, 部分模块为 0x3D)\n",
                   OLED_I2C_ADDRESS);
        delete _oled;  // 初始化失败即释放，无屏运行时不白占内存
        _oled = nullptr;
        _ok   = false;
        return false;
    }
    _ok = true;
    LOG_PRINTF("[OLED] init OK at 0x%02X (%ux%u)\n",
               OLED_I2C_ADDRESS, OLED_WIDTH, OLED_HEIGHT);
    splash();
    return true;
}

void DisplayService::splash() {
    _oled->clearDisplay();
    _oled->setTextColor(SSD1306_WHITE);
    _oled->setTextSize(2);
    _oled->setCursor(10, 8);
    _oled->print("Smart");
    _oled->setCursor(10, 28);
    _oled->print("Parking");
    _oled->setTextSize(1);
    _oled->setCursor(10, 52);
    _oled->print("booting...");
    _oled->display();
}

void DisplayService::update(uint32_t now, const ParkingStatus& st, const char* netInfo) {
    if (!_ok || _oled == nullptr || now - _lastDrawMs < DISPLAY_REFRESH_MS) {
        return;
    }
    _lastDrawMs = now;

    char line[24];
    _oled->clearDisplay();
    _oled->setTextSize(1);
    _oled->setTextColor(SSD1306_WHITE);

    _oled->setCursor(0, 0);
    _oled->print("   Smart Parking");
    _oled->drawFastHLine(0, 9, OLED_WIDTH, SSD1306_WHITE);

    snprintf(line, sizeof(line), "Free %u/%u", st.freeSlots, st.totalSlots);
    _oled->setCursor(0, 12);
    _oled->print(line);

    // 车位地图：X=占用，_=空闲
    _oled->setCursor(0, 21);
    for (uint8_t i = 0; i < st.totalSlots && i < MAX_PARKING_SLOTS; ++i) {
        snprintf(line, sizeof(line), "%u:%c ", i + 1, st.slotOccupied[i] ? 'X' : '_');
        _oled->print(line);
    }

    snprintf(line, sizeof(line), "Gate: %s", gateStateName(st.gateState));
    _oled->setCursor(0, 30);
    _oled->print(line);

    snprintf(line, sizeof(line), "Card: %.15s", st.lastCardUid);
    _oled->setCursor(0, 39);
    _oled->print(line);

    snprintf(line, sizeof(line), "%.21s", st.lastMessage);
    _oled->setCursor(0, 48);
    _oled->print(line);

    snprintf(line, sizeof(line), "%.21s", netInfo != nullptr ? netInfo : "");
    _oled->setCursor(0, 57);
    _oled->print(line);

    _oled->display();
}

#else  // !ENABLE_DISPLAY — 空实现，保证关闭功能后仍可编译

bool DisplayService::begin() {
    LOG_PRINTLN("[OLED] disabled by ENABLE_DISPLAY=0");
    return false;
}

void DisplayService::update(uint32_t now, const ParkingStatus& st, const char* netInfo) {
    (void)now;
    (void)st;
    (void)netInfo;
}

#endif  // ENABLE_DISPLAY
