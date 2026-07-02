#pragma once
#include <Arduino.h>
#include "../config/Settings.h"
#include "ParkingTypes.h"
#include "ParkingManager.h"

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
//  GET  /                 网页仪表盘（车位 + 计费）
//  GET  /api/status       系统状态 JSON
//  POST /api/reset        清零累计收入与最近记录、并重置在场车辆计时（无认证，
//                         由 ENABLE_WEB_MANUAL_CONTROL 控制，关闭后返回 403）
//  POST /api/sim?slot=N   仅 ENABLE_SIM_MODE=1 时存在：翻转第 N 个车位（模拟车辆
//                         进出），供无传感器演示 / 录视频使用
//  POST /api/gate?action=open|close
//                         仅 ENABLE_GATE=1 时存在：手动开闸 / 落闸（同样受
//                         ENABLE_WEB_MANUAL_CONTROL 控制，关闭后返回 403）
//
// 安全边界：接口无认证，切勿将本设备暴露到公共网络。
// =====================================================================
class WebDashboard {
public:
    void begin(ParkingManager* pm);
    void update(uint32_t now);

    // 给 OLED 显示的网络状态摘要，如 "IP 192.168.1.5" / "AP 192.168.4.1"
    const char* networkSummary() const;

private:
    ParkingManager* _pm = nullptr;

#if ENABLE_WEB
    // 注意：枚举值带 NET_ 前缀，避开 ESP32 core 的宏定义
    // （esp32-hal-gpio.h 中 DISABLED 是宏，裸名会被预处理器替换）
    enum class NetMode : uint8_t { NET_DISABLED, NET_CONNECTING, NET_STA_READY, NET_AP_READY };

    void startServer();
    void startApFallback();
    void handleRoot();
    void handleStatus();
    void handleReset();
#if ENABLE_SIM_MODE
    void handleSim();  // POST /api/sim?slot=N：演示模式下翻转车位（模拟车辆进出）
#endif
#if ENABLE_GATE
    void handleGate();  // POST /api/gate?action=open|close：手动开闸 / 落闸
#endif

    NetMode   _mode = NetMode::NET_DISABLED;
    WebServer _server{WEB_SERVER_PORT};
    bool      _serverStarted    = false;
    uint32_t  _connectStartMs   = 0;
    char      _netSummary[32]   = "Web: starting";
#endif
};
