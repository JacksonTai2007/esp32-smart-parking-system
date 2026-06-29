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
  :root { --bg:#0e1218; --card:#1b2230; --line:#2c3650; --txt:#e8edf6; --dim:#8b97ad;
          --ok:#3ddc84; --warn:#ffb648; --bad:#ff5d5d; --acc:#4da3ff;
          --road:#23272e; --paint:#c7cfdd; }
  * { box-sizing:border-box; margin:0; padding:0; }
  body { background:var(--bg); color:var(--txt);
         font-family:system-ui,"PingFang SC","Microsoft YaHei",sans-serif;
         max-width:680px; margin:0 auto; padding:16px; }
  h1 { font-size:1.25rem; margin-bottom:2px; }
  .sub { color:var(--dim); font-size:.8rem; margin-bottom:14px; }
  .badge { float:right; font-size:.75rem; padding:3px 10px; border-radius:99px; background:var(--card); }
  .badge.on { color:var(--ok); } .badge.off { color:var(--bad); }
  .tag { display:inline-block; font-size:.7rem; padding:1px 8px; border-radius:99px;
         background:#3a2d12; color:var(--warn); margin-left:6px; vertical-align:middle; }
  .stats { display:grid; grid-template-columns:repeat(3,1fr); gap:10px; margin-bottom:12px; }
  .stat { background:var(--card); border:1px solid var(--line); border-radius:12px; padding:12px; text-align:center; }
  .stat b { display:block; font-size:1.8rem; }
  .stat span { color:var(--dim); font-size:.78rem; }
  #stFree b { color:var(--ok); }
  #stRevenue b { color:var(--warn); }

  /* 停车场俯视平面图 */
  .lot { background:var(--road); border:1px solid var(--line); border-radius:14px;
         padding:14px 14px 12px; margin-bottom:12px;
         background-image:linear-gradient(180deg,rgba(255,255,255,.03),rgba(0,0,0,.12)); }
  .lothead { display:flex; justify-content:space-between; align-items:center;
             color:var(--dim); font-size:.82rem; margin-bottom:10px; }
  .lotbadge { font-size:.75rem; padding:2px 10px; border-radius:99px; font-weight:600; }
  .lotbadge.ok { color:var(--ok); background:rgba(61,220,132,.12); }
  .lotbadge.full { color:#fff; background:var(--bad); }
  .bays { display:grid; gap:8px; }
  .bay { position:relative; min-height:104px; border:2px solid var(--paint); border-top:none;
         border-radius:0 0 7px 7px; display:flex; flex-direction:column;
         align-items:center; justify-content:space-between; padding:7px 4px;
         background:rgba(0,0,0,.18); transition:.15s; }
  .bay.free { border-color:#56627c; border-style:dashed; }
  .bay.occ  { border-color:var(--acc); background:rgba(77,163,255,.12); }
  .bay .bno { align-self:flex-start; font-size:.74rem; color:var(--dim); font-weight:600; }
  .bay.occ .bno { color:var(--acc); }
  .bay .car { font-size:2.1rem; line-height:1; }
  .bay .binfo { font-size:.76rem; color:var(--dim); }
  .bay.occ .binfo { color:var(--warn); font-weight:600; }
  .lane { margin-top:8px; height:30px; border-top:2px dashed #d8b13a; border-radius:0 0 8px 8px;
          background:#15181d; display:flex; align-items:center; justify-content:center;
          color:var(--dim); font-size:.74rem; letter-spacing:1px; }

  .rule { color:var(--dim); font-size:.76rem; text-align:center; margin:-4px 0 12px; }
  .panel { background:var(--card); border:1px solid var(--line); border-radius:12px;
           padding:12px 14px; margin-bottom:12px; font-size:.9rem; }
  .panel h2 { font-size:.85rem; color:var(--dim); font-weight:600; margin-bottom:6px; }
  .row { display:flex; justify-content:space-between; padding:5px 0; border-bottom:1px dashed var(--line); }
  .row:last-child { border-bottom:none; }
  .row .k { color:var(--dim); }
  .fee { color:var(--warn); }
  .muted { color:var(--dim); }
  .simbtns { display:grid; grid-template-columns:repeat(2,1fr); gap:8px; }
  .gate { font-weight:600; }
  .gate.open { color:var(--ok); }
  .gate.closed { color:var(--dim); }
  .gatebtns { display:grid; grid-template-columns:repeat(2,1fr); gap:8px; }
  .gatebtn.open  { background:#1f6f43; }
  .gatebtn.close { background:#8a4b1d; }
  .btns { display:grid; grid-template-columns:1fr; gap:10px; margin-top:2px; }
  button { border:none; border-radius:10px; padding:11px 0; font-size:.92rem; cursor:pointer; color:#fff; }
  .simbtn.in  { background:#1f6f43; }
  .simbtn.out { background:#8a4b1d; }
  #btnReset { background:#33415e; }
  button:active { opacity:.75; }
  .foot { color:var(--dim); font-size:.72rem; text-align:center; margin-top:14px; }
</style>
</head>
<body>
  <span id="conn" class="badge off">连接中...</span>
  <h1>ESP32 智能停车场<span id="simTag" class="tag" style="display:none">模拟模式</span></h1>
  <div class="sub">Smart Parking System · 车位识别 + 计费 · 毕业设计演示</div>

  <div class="stats">
    <div class="stat" id="stTotal"><b>-</b><span>总车位</span></div>
    <div class="stat" id="stOcc"><b>-</b><span>已占用</span></div>
    <div class="stat" id="stFree"><b>-</b><span>剩余车位</span></div>
  </div>

  <div class="lot">
    <div class="lothead"><span>🅿 停车场平面图（俯视）</span><span id="lotState" class="lotbadge ok">—</span></div>
    <div class="bays" id="bays"></div>
    <div class="lane"><span>⟵ 入口 · 出口 ⟶</span><span id="gate" class="gate closed">道闸 —</span></div>
  </div>

  <div class="stats">
    <div class="stat" id="stRevenue"><b>-</b><span>累计收入</span></div>
    <div class="stat" id="stCount"><b>-</b><span>停车次数</span></div>
    <div class="stat" id="stRate"><b>-</b><span>计费单价</span></div>
  </div>
  <div class="rule" id="rule">—</div>

  <div class="panel" id="simPanel" style="display:none">
    <h2>演示控制 · 点击模拟车辆进出</h2>
    <div class="simbtns" id="simbtns"></div>
  </div>

  <div class="panel" id="gatePanel" style="display:none">
    <h2>道闸控制</h2>
    <div class="gatebtns">
      <button class="gatebtn open" onclick="gate('open')">⬆ 开闸</button>
      <button class="gatebtn close" onclick="gate('close')">⬇ 落闸</button>
    </div>
  </div>

  <div class="panel">
    <h2>最近停车记录</h2>
    <div id="recent"><div class="row muted">暂无记录</div></div>
  </div>

  <div class="panel">
    <div class="row"><span class="k">系统消息</span><span id="msg">-</span></div>
    <div class="row"><span class="k">运行时间</span><span id="up">-</span></div>
  </div>

  <div class="btns">
    <button id="btnReset" onclick="resetStats()">清零累计收入</button>
  </div>

  <div class="foot">每 1 秒自动刷新 · GET /api/status</div>

<script>
function fmtDur(ms) {
  var s = Math.floor(ms / 1000);
  var h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60), ss = s % 60;
  var pad = function (n) { return (n < 10 ? '0' : '') + n; };
  return (h > 0 ? h + ':' : '') + pad(m) + ':' + pad(ss);
}
function fmtMoney(cents, sym) { return sym + (cents / 100).toFixed(2); }

function render(st) {
  document.querySelector('#stTotal b').textContent = st.totalSlots;
  document.querySelector('#stOcc b').textContent   = st.occupiedSlots;
  document.querySelector('#stFree b').textContent  = st.freeSlots;
  document.querySelector('#stRevenue b').textContent = fmtMoney(st.totalRevenueCents, st.currency);
  document.querySelector('#stCount b').textContent   = st.sessionCount;
  document.querySelector('#stRate b').textContent    =
    fmtMoney(st.ratePerMinCents, st.currency) + '/min';
  document.getElementById('rule').textContent =
    '前 ' + st.freePeriodSec + ' 秒免费 · 超出按分钟向上取整 × ' +
    fmtMoney(st.ratePerMinCents, st.currency);

  var full = st.freeSlots === 0;
  var lb = document.getElementById('lotState');
  lb.textContent = full ? '车位已满' : ('剩余 ' + st.freeSlots + ' / ' + st.totalSlots);
  lb.className = 'lotbadge ' + (full ? 'full' : 'ok');

  document.getElementById('bays').style.gridTemplateColumns =
    'repeat(' + st.slots.length + ',1fr)';
  var html = '';
  for (var i = 0; i < st.slots.length; i++) {
    var s = st.slots[i];
    var occ = s.occupied;
    html += '<div class="bay ' + (occ ? 'occ' : 'free') + '">' +
            '<span class="bno">P' + (i + 1) + '</span>' +
            '<span class="car">' + (occ ? '🚗' : '') + '</span>' +
            '<span class="binfo">' + (occ ? fmtDur(s.durationMs) : '空闲') + '</span>' +
            '</div>';
  }
  document.getElementById('bays').innerHTML = html;

  var sp = document.getElementById('simPanel');
  document.getElementById('simTag').style.display = st.simMode ? '' : 'none';
  if (st.simMode) {
    sp.style.display = '';
    var sb = '';
    for (var k = 0; k < st.slots.length; k++) {
      var o = st.slots[k].occupied;
      sb += '<button class="simbtn ' + (o ? 'out' : 'in') + '" onclick="sim(' + (k + 1) + ')">' +
            'P' + (k + 1) + (o ? ' 🚙 驶离' : ' 🚗 驶入') + '</button>';
    }
    document.getElementById('simbtns').innerHTML = sb;
  } else {
    sp.style.display = 'none';
  }

  var g = document.getElementById('gate');
  var gp = document.getElementById('gatePanel');
  if (st.gateEnabled) {
    g.style.display = '';
    g.textContent = st.gateOpen ? '道闸 ▲ 已抬起' : '道闸 ▼ 已落下';
    g.className = 'gate ' + (st.gateOpen ? 'open' : 'closed');
    gp.style.display = '';
  } else {
    g.style.display = 'none';
    gp.style.display = 'none';
  }

  var r = '';
  if (st.recent && st.recent.length) {
    for (var j = 0; j < st.recent.length; j++) {
      var e = st.recent[j];
      r += '<div class="row"><span class="k">P' + e.slot + ' · ' + fmtDur(e.durationMs) +
           '</span><span class="fee">' + fmtMoney(e.feeCents, st.currency) + '</span></div>';
    }
  } else {
    r = '<div class="row muted">暂无记录</div>';
  }
  document.getElementById('recent').innerHTML = r;

  document.getElementById('msg').textContent = st.lastMessage;
  document.getElementById('up').textContent  = fmtDur(st.uptimeMs);
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

function sim(n) {
  fetch('/api/sim?slot=' + n, { method: 'POST' }).then(tick).catch(function () {});
}

function gate(action) {
  fetch('/api/gate?action=' + action, { method: 'POST' }).then(tick).catch(function () {});
}

function resetStats() {
  if (!confirm('确定清零累计收入与最近记录？')) { return; }
  fetch('/api/reset', { method: 'POST' }).then(tick).catch(function () {});
}

setInterval(tick, 1000);
tick();
</script>
</body>
</html>
)rawliteral";

void WebDashboard::begin(ParkingManager* pm) {
    _pm = pm;

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
    _server.on("/api/reset", HTTP_POST, [this]() { handleReset(); });
#if ENABLE_SIM_MODE
    _server.on("/api/sim", HTTP_POST, [this]() { handleSim(); });
#endif
#if ENABLE_GATE
    _server.on("/api/gate", HTTP_POST, [this]() { handleGate(); });
#endif
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

// JSON 字符串字段统一转义。当前字段（固件内置消息、货币符号）内容受控，
// 转义属防御式处理：后续往 JSON 里加动态文本时不会产生非法格式。
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
    const ParkingStatus st = _pm->status();

    String json;
    json.reserve(768);
    json += "{\"totalSlots\":";
    json += st.totalSlots;
    json += ",\"occupiedSlots\":";
    json += st.occupiedSlots;
    json += ",\"freeSlots\":";
    json += st.freeSlots;
    json += ",\"slots\":[";
    for (uint8_t i = 0; i < st.totalSlots && i < MAX_PARKING_SLOTS; ++i) {
        if (i > 0) json += ',';
        json += "{\"occupied\":";
        json += st.slotOccupied[i] ? "true" : "false";
        json += ",\"durationMs\":";
        json += st.slotDurationMs[i];
        json += '}';
    }
    json += "],\"totalRevenueCents\":";
    json += st.totalRevenueCents;
    json += ",\"sessionCount\":";
    json += st.sessionCount;
    json += ",\"ratePerMinCents\":";
    json += (uint32_t)PARKING_RATE_PER_MIN_CENTS;
    json += ",\"freePeriodSec\":";
    json += (uint32_t)PARKING_FREE_PERIOD_SEC;
    json += ",\"simMode\":";
#if ENABLE_SIM_MODE
    json += "true";
#else
    json += "false";
#endif
    json += ",\"gateEnabled\":";
#if ENABLE_GATE
    json += "true";
#else
    json += "false";
#endif
    json += ",\"gateOpen\":";
    json += st.gateOpen ? "true" : "false";
    json += ",\"currency\":\"";
    appendJsonEscaped(json, CURRENCY_SYMBOL);
    json += "\",\"recent\":[";
    for (uint8_t i = 0; i < st.recentCount && i < MAX_SESSION_LOG; ++i) {
        if (i > 0) json += ',';
        json += "{\"slot\":";
        json += st.recent[i].slotId;
        json += ",\"durationMs\":";
        json += st.recent[i].durationMs;
        json += ",\"feeCents\":";
        json += st.recent[i].feeCents;
        json += '}';
    }
    json += "],\"lastMessage\":\"";
    appendJsonEscaped(json, st.lastMessage);
    json += "\",\"uptimeMs\":";
    json += st.uptimeMs;
    json += '}';

    _server.send(200, "application/json", json);
}

// 管理接口无认证，安全边界见 Settings.h 的 ENABLE_WEB_MANUAL_CONTROL 注释；
// 关闭开关后返回 403。
void WebDashboard::handleReset() {
#if ENABLE_WEB_MANUAL_CONTROL
    _pm->resetStats();
    _server.send(200, "application/json", "{\"ok\":true,\"action\":\"reset\"}");
#else
    _server.send(403, "application/json", "{\"ok\":false,\"error\":\"manual control disabled\"}");
#endif
}

#if ENABLE_SIM_MODE
// 演示模式接口：POST /api/sim?slot=N 翻转第 N 个车位（模拟车辆驶入 / 驶离），
// 下一轮主循环 ParkingManager::update 会按真实进出流程识别并计费。
void WebDashboard::handleSim() {
    if (!_server.hasArg("slot")) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing slot\"}");
        return;
    }
    const int slot = _server.arg("slot").toInt();  // 1 起编号
    if (slot < 1 || slot > TOTAL_PARKING_SLOTS) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"slot out of range\"}");
        return;
    }
    _pm->simToggleSlot((uint8_t)(slot - 1));
    _server.send(200, "application/json", "{\"ok\":true}");
}
#endif

#if ENABLE_GATE
// 手动道闸接口：POST /api/gate?action=open|close。与进出场自动抬杆并行；
// 同样受 ENABLE_WEB_MANUAL_CONTROL 控制（关闭后只读，返回 403）。
void WebDashboard::handleGate() {
#if ENABLE_WEB_MANUAL_CONTROL
    const String action = _server.arg("action");
    if (action == "open") {
        _pm->gateOpenManual();
    } else if (action == "close") {
        _pm->gateCloseManual();
    } else {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"action must be open|close\"}");
        return;
    }
    _server.send(200, "application/json", "{\"ok\":true}");
#else
    _server.send(403, "application/json", "{\"ok\":false,\"error\":\"manual control disabled\"}");
#endif
}
#endif

#else  // !ENABLE_WEB — 空实现，保证关闭功能后仍可编译

void WebDashboard::begin(ParkingManager* pm) {
    _pm = pm;
    LOG_PRINTLN("[Web] disabled by ENABLE_WEB=0");
}

void WebDashboard::update(uint32_t now) {
    (void)now;
}

const char* WebDashboard::networkSummary() const {
    return "Web: disabled";
}

#endif  // ENABLE_WEB
