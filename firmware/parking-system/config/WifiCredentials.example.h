#pragma once

// =====================================================================
// Wi-Fi 凭据模板
//
// 使用方法（仅在你本地操作）：
//   1. 复制本文件为同目录下的 WifiCredentials.h
//        cp WifiCredentials.example.h WifiCredentials.h
//   2. 填入真实的 Wi-Fi 名称和密码
//   3. WifiCredentials.h 已被 .gitignore 忽略，绝不会被提交到仓库
//
// 如果不创建 WifiCredentials.h：
//   - 固件仍然可以正常编译（CI 就是在无此文件的情况下编译的）
//   - WIFI_AP_FALLBACK=1 时 ESP32 会自动开启热点模式提供网页
//   - WIFI_AP_FALLBACK=0 时 Web 功能提示未启用
// =====================================================================

#define WIFI_STA_SSID     "your-wifi-name"
#define WIFI_STA_PASSWORD "your-wifi-password"
