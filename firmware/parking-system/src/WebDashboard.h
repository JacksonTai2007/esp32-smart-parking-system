#pragma once
#include <Arduino.h>
#include "../config/Settings.h"
#include "ParkingTypes.h"
#include "ParkingStateMachine.h"

#if ENABLE_WEB
#include <WiFi.h>
#include <WebServer.h>
#endif

// =====================================================================
// WebDashboard — Wi-Fi 连接 + 网页仪表盘 + JSON API
//
// 网络策略（全部非阻塞）：
//  - 存在 config/WifiCredentials.h（git 忽略）时，以 STA 模式连接路由器
//  - 连接超时或没有凭据文件时：
//      WIFI_AP_FALLBACK=1 -> 自动开热点（答辩现场无 Wi-Fi 的兜底方案）
//      WIFI_AP_FALLBACK=0 -> Web 功能提示未启用，系统其余功能不受影响
//  - 没有凭据文件时固件必须照常编译（CI 即此场景）
//
// HTTP 接口：
//  GET  /                 网页仪表盘
//  GET  /api/status       系统状态 JSON
//  POST /api/gate/open    手动开闸
//  POST /api/gate/close   手动关闸
//  POST /api/reset        重置演示状态
// =====================================================================
class WebDashboard {
public:
    void begin(ParkingStateMachine* sm);
    void update(uint32_t now);

    // 给 OLED 显示的网络状态摘要，如 "IP 192.168.1.5" / "AP 192.168.4.1"
    const char* networkSummary() const;

private:
    ParkingStateMachine* _sm = nullptr;

#if ENABLE_WEB
    // 注意：枚举值带 NET_ 前缀，避开 ESP32 core 的宏定义
    // （esp32-hal-gpio.h 中 DISABLED 是宏，裸名会被预处理器替换）
    enum class NetMode : uint8_t { NET_DISABLED, NET_CONNECTING, NET_STA_READY, NET_AP_READY };

    void startServer();
    void startApFallback();
    void handleRoot();
    void handleStatus();
    void handleGateOpen();
    void handleGateClose();
    void handleReset();

    NetMode   _mode = NetMode::NET_DISABLED;
    WebServer _server{WEB_SERVER_PORT};
    bool      _serverStarted    = false;
    uint32_t  _connectStartMs   = 0;
    char      _netSummary[32]   = "Web: starting";
#endif
};
