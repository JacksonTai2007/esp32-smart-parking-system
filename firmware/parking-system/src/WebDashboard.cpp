#include "WebDashboard.h"

#if ENABLE_WEB

// 真实 Wi-Fi 凭据文件可选存在（被 .gitignore 忽略）；
// 不存在时走 AP 兜底或禁用 Web，固件照常编译。
#if defined(__has_include)
#if __has_include("../config/WifiCredentials.h")
#include "../config/WifiCredentials.h"
#define HAS_WIFI_CREDENTIALS 1
#endif
#endif

// ---------------------------------------------------------------
// 内嵌网页（与 web/dashboard.html 保持一致，改动需双向同步）
// ---------------------------------------------------------------
static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 智能停车场</title>
<style>
  :root { --bg:#10141c; --card:#1b2230; --line:#2c3650; --txt:#e8edf6; --dim:#8b97ad;
          --ok:#3ddc84; --warn:#ffb648; --bad:#ff5d5d; --acc:#4da3ff; }
  * { box-sizing:border-box; margin:0; padding:0; }
  body { background:var(--bg); color:var(--txt); font-family:system-ui,"PingFang SC","Microsoft YaHei",sans-serif;
         max-width:680px; margin:0 auto; padding:16px; }
  h1 { font-size:1.25rem; margin-bottom:2px; }
  .sub { color:var(--dim); font-size:.8rem; margin-bottom:14px; }
  .badge { float:right; font-size:.75rem; padding:3px 10px; border-radius:99px; background:var(--card); }
  .badge.on { color:var(--ok); } .badge.off { color:var(--bad); }
  .stats { display:grid; grid-template-columns:repeat(3,1fr); gap:10px; margin-bottom:12px; }
  .stat { background:var(--card); border:1px solid var(--line); border-radius:12px; padding:12px; text-align:center; }
  .stat b { display:block; font-size:1.8rem; }
  .stat span { color:var(--dim); font-size:.78rem; }
  #stFree b { color:var(--ok); }
  .slots { display:grid; grid-template-columns:repeat(4,1fr); gap:10px; margin-bottom:12px; }
  .slot { border-radius:12px; padding:14px 6px; text-align:center; font-weight:600;
          background:var(--card); border:1px solid var(--line); }
  .slot.free { border-color:var(--ok); color:var(--ok); }
  .slot.occ  { border-color:var(--bad); color:var(--bad); }
  .slot small { display:block; font-weight:400; font-size:.72rem; margin-top:4px; color:var(--dim); }
  .panel { background:var(--card); border:1px solid var(--line); border-radius:12px;
           padding:12px 14px; margin-bottom:12px; font-size:.9rem; }
  .row { display:flex; justify-content:space-between; padding:5px 0; border-bottom:1px dashed var(--line); }
  .row:last-child { border-bottom:none; }
  .row .k { color:var(--dim); }
  .gate-open { color:var(--ok); } .gate-closed { color:var(--dim); }
  .gate-opening, .gate-closing { color:var(--warn); }
  .btns { display:grid; grid-template-columns:repeat(3,1fr); gap:10px; }
  button { border:none; border-radius:10px; padding:12px 0; font-size:.95rem; cursor:pointer; color:#fff; }
  #btnOpen { background:#1f7a4d; } #btnClose { background:#7a1f2b; } #btnReset { background:#33415e; }
  button:active { opacity:.75; }
  .foot { color:var(--dim); font-size:.72rem; text-align:center; margin-top:14px; }
</style>
</head>
<body>
  <span id="conn" class="badge off">连接中...</span>
  <h1>ESP32 智能停车场</h1>
  <div class="sub">Smart Parking System · 毕业设计演示</div>

  <div class="stats">
    <div class="stat" id="stTotal"><b>-</b><span>总车位</span></div>
    <div class="stat" id="stOcc"><b>-</b><span>已占用</span></div>
    <div class="stat" id="stFree"><b>-</b><span>剩余车位</span></div>
  </div>

  <div class="slots" id="slots"></div>

  <div class="panel">
    <div class="row"><span class="k">闸机状态</span><span id="gate">-</span></div>
    <div class="row"><span class="k">入口状态</span><span id="entry">-</span></div>
    <div class="row"><span class="k">最近刷卡 UID</span><span id="card">-</span></div>
    <div class="row"><span class="k">系统消息</span><span id="msg">-</span></div>
    <div class="row"><span class="k">运行时间</span><span id="up">-</span></div>
  </div>

  <div class="btns">
    <button id="btnOpen"  onclick="post('/api/gate/open')">手动开闸</button>
    <button id="btnClose" onclick="post('/api/gate/close')">手动关闸</button>
    <button id="btnReset" onclick="post('/api/reset')">重置演示</button>
  </div>

  <div class="foot">每 1 秒自动刷新 · GET /api/status</div>

<script>
var GATE_CN  = { closed:'已关闭', opening:'开闸中', open:'已开启', closing:'关闸中' };
var ENTRY_CN = { idle:'空闲', vehicle_detected:'检测到车辆', waiting_for_card:'等待刷卡',
                 card_accepted:'刷卡成功', card_rejected:'无效卡', parking_full:'车位已满' };

function fmtUptime(ms) {
  var s = Math.floor(ms / 1000);
  var h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60);
  return h + 'h ' + m + 'm ' + (s % 60) + 's';
}

function render(st) {
  document.querySelector('#stTotal b').textContent = st.totalSlots;
  document.querySelector('#stOcc b').textContent   = st.occupiedSlots;
  document.querySelector('#stFree b').textContent  = st.freeSlots;

  var html = '';
  for (var i = 0; i < st.slots.length; i++) {
    var occ = st.slots[i];
    html += '<div class="slot ' + (occ ? 'occ' : 'free') + '">P' + (i + 1) +
            '<small>' + (occ ? '占用' : '空闲') + '</small></div>';
  }
  document.getElementById('slots').innerHTML = html;

  var g = document.getElementById('gate');
  g.textContent = GATE_CN[st.gateState] || st.gateState;
  g.className = 'gate-' + st.gateState;
  document.getElementById('entry').textContent = ENTRY_CN[st.entryState] || st.entryState;
  document.getElementById('card').textContent  = st.lastCardUid;
  document.getElementById('msg').textContent   = st.lastMessage;
  document.getElementById('up').textContent    = fmtUptime(st.uptimeMs);
}

function tick() {
  fetch('/api/status').then(function (r) { return r.json(); }).then(function (st) {
    render(st);
    var c = document.getElementById('conn');
    c.textContent = '在线'; c.className = 'badge on';
  }).catch(function () {
    var c = document.getElementById('conn');
    c.textContent = '离线'; c.className = 'badge off';
  });
}

function post(path) {
  fetch(path, { method: 'POST' }).then(tick).catch(function () {});
}

setInterval(tick, 1000);
tick();
</script>
</body>
</html>
)rawliteral";

void WebDashboard::begin(ParkingStateMachine* sm) {
    _sm = sm;

#ifdef HAS_WIFI_CREDENTIALS
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
    _mode           = NetMode::NET_CONNECTING;
    _connectStartMs = millis();
    strlcpy(_netSummary, "WiFi connecting", sizeof(_netSummary));
    LOG_PRINTF("[Web] connecting to Wi-Fi \"%s\" ...\n", WIFI_STA_SSID);
#else
    LOG_PRINTLN("[Web] config/WifiCredentials.h not found (see WifiCredentials.example.h)");
#if WIFI_AP_FALLBACK
    startApFallback();
#else
    _mode = NetMode::NET_DISABLED;
    strlcpy(_netSummary, "Web: no config", sizeof(_netSummary));
    LOG_PRINTLN("[Web] dashboard disabled (no credentials, AP fallback off)");
#endif
#endif
}

void WebDashboard::startApFallback() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    _mode = NetMode::NET_AP_READY;
    startServer();
    snprintf(_netSummary, sizeof(_netSummary), "AP %s", WiFi.softAPIP().toString().c_str());
    LOG_PRINTF("[Web] AP fallback up: SSID=\"%s\" pass=\"%s\" -> http://%s/\n",
               WIFI_AP_SSID, WIFI_AP_PASSWORD, WiFi.softAPIP().toString().c_str());
}

void WebDashboard::startServer() {
    if (_serverStarted) {
        return;
    }
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/api/gate/open", HTTP_POST, [this]() { handleGateOpen(); });
    _server.on("/api/gate/close", HTTP_POST, [this]() { handleGateClose(); });
    _server.on("/api/reset", HTTP_POST, [this]() { handleReset(); });
    _server.onNotFound([this]() {
        _server.send(404, "application/json", "{\"error\":\"not found\"}");
    });
    _server.begin();
    _serverStarted = true;
    LOG_PRINTF("[Web] HTTP server started on port %u\n", WEB_SERVER_PORT);
}

void WebDashboard::update(uint32_t now) {
    switch (_mode) {
        case NetMode::NET_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                _mode = NetMode::NET_STA_READY;
                startServer();
                snprintf(_netSummary, sizeof(_netSummary), "IP %s",
                         WiFi.localIP().toString().c_str());
                LOG_PRINTF("[Web] Wi-Fi connected -> http://%s/\n",
                           WiFi.localIP().toString().c_str());
            } else if (now - _connectStartMs >= WIFI_CONNECT_TIMEOUT_MS) {
                LOG_PRINTLN("[Web] Wi-Fi connect timeout");
#if WIFI_AP_FALLBACK
                startApFallback();
#else
                _mode = NetMode::NET_DISABLED;
                strlcpy(_netSummary, "Web: offline", sizeof(_netSummary));
                LOG_PRINTLN("[Web] dashboard disabled (connect failed, AP fallback off)");
#endif
            }
            break;

        case NetMode::NET_STA_READY:
        case NetMode::NET_AP_READY:
            _server.handleClient();
            break;

        case NetMode::NET_DISABLED:
            break;
    }
}

const char* WebDashboard::networkSummary() const {
    return _netSummary;
}

void WebDashboard::handleRoot() {
    _server.send_P(200, "text/html", DASHBOARD_HTML);
}

void WebDashboard::handleStatus() {
    const ParkingStatus st = _sm->status();

    String json;
    json.reserve(512);
    json += "{\"totalSlots\":";
    json += st.totalSlots;
    json += ",\"occupiedSlots\":";
    json += st.occupiedSlots;
    json += ",\"freeSlots\":";
    json += st.freeSlots;
    json += ",\"slots\":[";
    for (uint8_t i = 0; i < st.totalSlots && i < MAX_PARKING_SLOTS; ++i) {
        if (i > 0) json += ',';
        json += st.slotOccupied[i] ? "true" : "false";
    }
    json += "],\"gateState\":\"";
    json += gateStateName(st.gateState);
    json += "\",\"entryState\":\"";
    json += entryStateName(st.entryState);
    json += "\",\"lastCardUid\":\"";
    json += st.lastCardUid;  // UID 为受控的十六进制字符串，无需转义
    json += "\",\"lastMessage\":\"";
    json += st.lastMessage;  // 消息为固件内置英文常量，无需转义
    json += "\",\"alarmActive\":";
    json += st.alarmActive ? "true" : "false";
    json += ",\"uptimeMs\":";
    json += st.uptimeMs;
    json += '}';

    _server.send(200, "application/json", json);
}

void WebDashboard::handleGateOpen() {
    _sm->manualOpenGate();
    _server.send(200, "application/json", "{\"ok\":true,\"action\":\"gate_open\"}");
}

void WebDashboard::handleGateClose() {
    _sm->manualCloseGate();
    _server.send(200, "application/json", "{\"ok\":true,\"action\":\"gate_close\"}");
}

void WebDashboard::handleReset() {
    _sm->resetDemo();
    _server.send(200, "application/json", "{\"ok\":true,\"action\":\"reset\"}");
}

#else  // !ENABLE_WEB — 空实现，保证关闭功能后仍可编译

void WebDashboard::begin(ParkingStateMachine* sm) {
    _sm = sm;
    LOG_PRINTLN("[Web] disabled by ENABLE_WEB=0");
}

void WebDashboard::update(uint32_t now) {
    (void)now;
}

const char* WebDashboard::networkSummary() const {
    return "Web: disabled";
}

#endif  // ENABLE_WEB
