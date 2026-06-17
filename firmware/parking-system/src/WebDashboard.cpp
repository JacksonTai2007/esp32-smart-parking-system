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
<title>智慧停车控制台</title>
<style>
  :root{
    --bg:#eef2f7; --card:#fff; --ink:#1f2733; --dim:#6b7787; --line:#e6ebf2;
    --free:#16b364; --occ:#e5484d; --warn:#f5a524; --accent:#4f6bed; --accent2:#7c4dff;
  }
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:var(--bg);color:var(--ink);
    font-family:system-ui,-apple-system,"PingFang SC","Microsoft YaHei",sans-serif;
    max-width:760px;margin:0 auto;padding:18px 16px 40px}
  header{display:flex;align-items:center;justify-content:space-between;margin-bottom:18px}
  .brand{display:flex;align-items:center;gap:10px;font-size:1.25rem;font-weight:700}
  .brand .logo{width:34px;height:34px;border-radius:10px;display:grid;place-items:center;
    color:#fff;font-weight:800;background:linear-gradient(135deg,var(--accent),var(--accent2))}
  .meta{text-align:right;font-size:.72rem;color:var(--dim);line-height:1.6}
  .pill{display:inline-block;padding:2px 9px;border-radius:99px;font-size:.72rem;font-weight:600}
  .pill.on{background:#e7f8ef;color:var(--free)} .pill.off{background:#fdecec;color:var(--occ)}
  .card{background:var(--card);border:1px solid var(--line);border-radius:16px;
    padding:18px;margin-bottom:14px;box-shadow:0 1px 2px rgba(20,30,50,.04)}
  .hero{display:flex;align-items:center;gap:22px;flex-wrap:wrap}
  .ring{position:relative;width:140px;height:140px;flex:0 0 auto}
  .ring svg{transform:rotate(-90deg)}
  .ring .ctr{position:absolute;inset:0;display:grid;place-content:center;text-align:center}
  .ring .ctr b{font-size:2.4rem;line-height:1;color:var(--free)}
  .ring .ctr span{font-size:.72rem;color:var(--dim)}
  .chips{display:flex;gap:10px;flex:1;min-width:200px}
  .chip{flex:1;text-align:center;background:#f7f9fc;border:1px solid var(--line);
    border-radius:12px;padding:14px 6px}
  .chip b{display:block;font-size:1.5rem} .chip span{font-size:.72rem;color:var(--dim)}
  .chip.free b{color:var(--free)} .chip.occ b{color:var(--occ)}
  .title{font-size:.78rem;color:var(--dim);font-weight:600;margin-bottom:12px;
    text-transform:uppercase;letter-spacing:.05em}
  .lot{display:grid;grid-template-columns:repeat(auto-fit,minmax(70px,1fr));gap:12px}
  .stall{border-radius:12px;padding:12px 8px;text-align:center;border:2px solid;transition:.2s}
  .stall.free{border-color:#bfe9cf;background:#f1fbf5;color:var(--free)}
  .stall.occ{border-color:#f6c9cb;background:#fdf2f2;color:var(--occ)}
  .stall .car{font-size:1.4rem;height:1.6rem;line-height:1.6rem}
  .stall .lbl{font-weight:700;margin-top:2px}
  .stall .st{font-size:.68rem;color:var(--dim);font-weight:500}
  .rows .row{display:flex;justify-content:space-between;align-items:center;
    padding:9px 0;border-bottom:1px dashed var(--line)}
  .rows .row:last-child{border-bottom:none}
  .row .k{color:var(--dim);font-size:.85rem}
  .tag{padding:3px 11px;border-radius:99px;font-size:.8rem;font-weight:600;background:#eef1f6;color:var(--ink)}
  .tag.g-open{background:#e7f8ef;color:var(--free)}
  .tag.g-closed{background:#eef1f6;color:var(--dim)}
  .tag.g-move{background:#fef4e6;color:var(--warn)}
  .msg{font-weight:600}
  .btns{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
  button{border:none;border-radius:12px;padding:13px 0;font-size:.95rem;font-weight:600;
    color:#fff;cursor:pointer;transition:.15s}
  button:active{transform:translateY(1px)}
  #bOpen{background:linear-gradient(135deg,#16b364,#0e9f74)}
  #bClose{background:linear-gradient(135deg,#e5484d,#c93b48)}
  #bReset{background:#5b6677}
  .foot{text-align:center;color:var(--dim);font-size:.72rem;margin-top:16px}
</style>
</head>
<body>
  <header>
    <div class="brand"><span class="logo">P</span>智慧停车控制台</div>
    <div class="meta">
      <span id="conn" class="pill off">连接中…</span><br>
      运行 <span id="up">--</span>
    </div>
  </header>

  <div class="card hero">
    <div class="ring">
      <svg width="140" height="140">
        <circle cx="70" cy="70" r="60" fill="none" stroke="#eef1f6" stroke-width="14"/>
        <circle id="arc" cx="70" cy="70" r="60" fill="none" stroke="#16b364" stroke-width="14"
                stroke-linecap="round" stroke-dasharray="377" stroke-dashoffset="377"/>
      </svg>
      <div class="ctr"><b id="freeBig">-</b><span>可用车位</span></div>
    </div>
    <div class="chips">
      <div class="chip"><b id="cTotal">-</b><span>总车位</span></div>
      <div class="chip occ"><b id="cOcc">-</b><span>已占用</span></div>
      <div class="chip free"><b id="cFree">-</b><span>剩余</span></div>
    </div>
  </div>

  <div class="card">
    <div class="title">车位状态</div>
    <div class="lot" id="lot"></div>
  </div>

  <div class="card">
    <div class="title">运行状态</div>
    <div class="rows">
      <div class="row"><span class="k">闸机</span><span id="gate" class="tag">-</span></div>
      <div class="row"><span class="k">入口流程</span><span id="entry" class="tag">-</span></div>
      <div class="row" id="cardRow" style="display:none"><span class="k">最近刷卡</span><span id="card" class="tag">-</span></div>
      <div class="row"><span class="k">系统消息</span><span id="msg" class="msg">-</span></div>
    </div>
  </div>

  <div class="card">
    <div class="title">手动控制</div>
    <div class="btns">
      <button id="bOpen" onclick="post('/api/gate/open')">开闸</button>
      <button id="bClose" onclick="post('/api/gate/close')">关闸</button>
      <button id="bReset" onclick="post('/api/reset')">重置</button>
    </div>
  </div>

  <div class="foot">每秒自动刷新 · ESP32 智能停车场 · GET /api/status</div>

<script>
var C = 377; // 2*pi*60，与 SVG 圆环半径一致
var GATE = { closed:['已关闭','g-closed'], opening:['开闸中','g-move'],
             open:['已开启','g-open'], closing:['关闸中','g-move'] };
var ENTRY = { idle:'空闲', vehicle_detected:'检测到车辆', waiting_for_card:'等待刷卡',
              admitted:'放行入场', card_rejected:'无效卡', parking_full:'车位已满' };

function fmtUptime(ms) {
  var s = Math.floor(ms / 1000), h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60);
  return h + 'h ' + m + 'm ' + (s % 60) + 's';
}

function render(st) {
  document.getElementById('cTotal').textContent  = st.totalSlots;
  document.getElementById('cOcc').textContent    = st.occupiedSlots;
  document.getElementById('cFree').textContent   = st.freeSlots;
  document.getElementById('freeBig').textContent = st.freeSlots;

  var ratio = st.totalSlots ? st.freeSlots / st.totalSlots : 0;
  document.getElementById('arc').setAttribute('stroke-dashoffset', (C * (1 - ratio)).toFixed(1));

  var lot = '';
  for (var i = 0; i < st.slots.length; i++) {
    var occ = st.slots[i];
    lot += '<div class="stall ' + (occ ? 'occ' : 'free') + '">' +
           '<div class="car">' + (occ ? '🚗' : '') + '</div>' +
           '<div class="lbl">P' + (i + 1) + '</div>' +
           '<div class="st">' + (occ ? '占用' : '空闲') + '</div></div>';
  }
  document.getElementById('lot').innerHTML = lot;

  var g = GATE[st.gateState] || [st.gateState, ''];
  var ge = document.getElementById('gate');
  ge.textContent = g[0]; ge.className = 'tag ' + g[1];
  document.getElementById('entry').textContent = ENTRY[st.entryState] || st.entryState;
  document.getElementById('msg').textContent   = st.lastMessage;
  document.getElementById('up').textContent    = fmtUptime(st.uptimeMs);

  // 刷卡 UID 只在启用 RFID（有真实卡号）时显示
  var cr = document.getElementById('cardRow');
  if (st.lastCardUid && st.lastCardUid !== '--') {
    cr.style.display = ''; document.getElementById('card').textContent = st.lastCardUid;
  } else {
    cr.style.display = 'none';
  }
}

function tick() {
  fetch('/api/status').then(function (r) { return r.json(); }).then(function (st) {
    render(st);
    var c = document.getElementById('conn'); c.textContent = '在线'; c.className = 'pill on';
  }).catch(function () {
    var c = document.getElementById('conn'); c.textContent = '离线'; c.className = 'pill off';
  });
}

function post(path) { fetch(path, { method: 'POST' }).then(tick).catch(function () {}); }

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
            // 运行中掉线：回到 CONNECTING 等待自动重连，
            // 超时后仍会走 AP fallback / 禁用分支，不会卡在旧 IP 上
            if (WiFi.status() != WL_CONNECTED) {
                _mode           = NetMode::NET_CONNECTING;
                _connectStartMs = now;
                strlcpy(_netSummary, "WiFi reconnecting", sizeof(_netSummary));
                LOG_PRINTLN("[Web] Wi-Fi connection lost, waiting for reconnect");
                break;
            }
            _server.handleClient();
            break;

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

// JSON 字符串字段统一转义。当前字段（UID 十六进制、固件内置消息）内容
// 受控，转义属防御式处理：后续往 JSON 里加动态文本时不会产生非法格式。
static void appendJsonEscaped(String& out, const char* s) {
    for (; *s != '\0'; ++s) {
        switch (*s) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += *s;     break;
        }
    }
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
    appendJsonEscaped(json, st.lastCardUid);
    json += "\",\"lastMessage\":\"";
    appendJsonEscaped(json, st.lastMessage);
    json += "\",\"alarmActive\":";
    json += st.alarmActive ? "true" : "false";
    json += ",\"uptimeMs\":";
    json += st.uptimeMs;
    json += '}';

    _server.send(200, "application/json", json);
}

// 手动控制接口无认证，安全边界见 Settings.h 的
// ENABLE_WEB_MANUAL_GATE_CONTROL 注释；关闭开关后统一返回 403。
void WebDashboard::handleGateOpen() {
#if ENABLE_WEB_MANUAL_GATE_CONTROL
    _sm->manualOpenGate();
    _server.send(200, "application/json", "{\"ok\":true,\"action\":\"gate_open\"}");
#else
    _server.send(403, "application/json", "{\"ok\":false,\"error\":\"manual control disabled\"}");
#endif
}

void WebDashboard::handleGateClose() {
#if ENABLE_WEB_MANUAL_GATE_CONTROL
    _sm->manualCloseGate();
    _server.send(200, "application/json", "{\"ok\":true,\"action\":\"gate_close\"}");
#else
    _server.send(403, "application/json", "{\"ok\":false,\"error\":\"manual control disabled\"}");
#endif
}

void WebDashboard::handleReset() {
#if ENABLE_WEB_MANUAL_GATE_CONTROL
    _sm->resetDemo();
    _server.send(200, "application/json", "{\"ok\":true,\"action\":\"reset\"}");
#else
    _server.send(403, "application/json", "{\"ok\":false,\"error\":\"manual control disabled\"}");
#endif
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
