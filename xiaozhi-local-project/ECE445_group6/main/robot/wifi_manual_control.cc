#include "wifi_manual_control.h"

#include <cstring>
#include <string>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"

namespace robot {

static constexpr const char* TAG = "wifi_manual";
static constexpr const char* AP_PASS = "deskbot123";   // >=8 chars for WPA2
static constexpr uint8_t    AP_CHANNEL = 6;
static constexpr uint8_t    AP_MAX_CONN = 2;

// Single-page UI served at http://192.168.4.1/. Buttons POST /cmd?m=<MotionCmd>;
// "AI Mode" POSTs /cmd?mode=ai. Visual style matches the Mac dashboard
// (dark theme, rounded cards, magenta+blue accents) so the brand
// language stays consistent whether the user is on Path A (this page,
// SoftAP) or Path B (the Mac panel, AI mode). Kept as a raw C++ string
// literal so it lives in flash and ships in one HTTP response.
static const char kIndexHtml[] = R"HTML(<!doctype html><html lang=en><head>
<meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>DeskBot Manual</title>
<style>
:root{
 --bg:#0b1020;--surface:#111729;--surface-2:#0f1628;--border:#1f2940;
 --text:#e6edf7;--text-mute:#9aa6bf;--text-soft:#6b7795;
 --primary:#60a5fa;--primary-soft:rgba(96,165,250,.18);
 --danger:#f87171;--danger-soft:rgba(248,113,113,.18);
 --magenta:#e879f9;--magenta-soft:rgba(232,121,249,.18);
 --success:#34d399;--success-soft:rgba(52,211,153,.18);
 --t:200ms cubic-bezier(.2,.8,.2,1);
}
*{box-sizing:border-box}
html,body{margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,"Inter","Segoe UI",sans-serif;
 background:var(--bg);color:var(--text);min-height:100vh;
 -webkit-font-smoothing:antialiased;padding:18px;
 display:flex;flex-direction:column;align-items:center;gap:14px}
.brand{display:flex;align-items:center;gap:12px;width:100%;max-width:380px}
.brand-mark{width:40px;height:40px;border-radius:12px;
 background:linear-gradient(135deg,var(--primary),var(--magenta));
 display:grid;place-items:center;color:#06101f;font-weight:800;font-size:18px}
.brand .t1{font-weight:700;font-size:16px;line-height:1.1}
.brand .t2{font-size:11.5px;color:var(--text-mute);margin-top:3px}
.card{background:var(--surface);border:1px solid var(--border);
 border-radius:16px;padding:16px;width:100%;max-width:380px;
 box-shadow:0 12px 32px rgba(0,0,0,.4)}
.label{font-size:10.5px;letter-spacing:.08em;text-transform:uppercase;
 color:var(--text-mute);font-weight:600;margin:0 0 10px}
.row{display:flex;align-items:center;justify-content:space-between}
.dot{display:inline-block;width:9px;height:9px;border-radius:50%;
 background:var(--success);margin-right:7px;
 animation:pulse 2.2s ease-in-out infinite;vertical-align:middle}
@keyframes pulse{0%,100%{box-shadow:0 0 0 0 rgba(52,211,153,.55)}
 70%{box-shadow:0 0 0 10px transparent}}
.pill{display:inline-flex;align-items:center;gap:6px;padding:5px 11px;
 border-radius:99px;background:var(--magenta-soft);color:var(--magenta);
 font-size:12px;font-weight:600}
.dpad{display:grid;grid-template-columns:repeat(3,1fr);
 grid-template-rows:repeat(3,1fr);gap:8px;aspect-ratio:1}
.btn{border:1px solid var(--border);background:var(--surface-2);
 color:var(--text);font:inherit;font-weight:600;font-size:24px;
 border-radius:12px;cursor:pointer;display:grid;place-items:center;
 transition:transform 150ms,background var(--t),color var(--t),
 border-color var(--t),box-shadow var(--t);
 -webkit-tap-highlight-color:transparent;user-select:none;
 -webkit-user-select:none;touch-action:manipulation}
.btn:active{transform:scale(.94)}
.btn:hover{background:var(--primary-soft);color:var(--primary);
 border-color:var(--primary)}
.btn.empty{background:transparent;border:none;pointer-events:none;visibility:hidden}
.btn.stop{background:var(--danger-soft);color:var(--danger);
 border-color:var(--danger);font-weight:700;font-size:16px;letter-spacing:.06em}
.btn.spin{font-size:22px;color:var(--magenta)}
.btn.spin:hover{background:var(--magenta-soft);border-color:var(--magenta);color:var(--magenta)}
.btn.primary{background:var(--primary);color:#06101f;border-color:var(--primary);
 font-size:14.5px;font-weight:700;height:48px}
.btn.primary:hover{filter:brightness(1.08)}
.toast{position:fixed;bottom:18px;left:50%;
 transform:translateX(-50%) translateY(8px);
 background:var(--surface);border:1px solid var(--border);
 border-radius:12px;padding:10px 16px;font-size:13px;
 opacity:0;transition:opacity 200ms,transform 200ms;
 pointer-events:none;z-index:1000;box-shadow:0 12px 24px rgba(0,0,0,.4)}
.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
.toast.error{color:var(--danger);border-color:var(--danger)}
.foot{font-size:11px;color:var(--text-soft);text-align:center;margin-top:4px}
</style></head>
<body>

<div class=brand>
 <div class=brand-mark>J</div>
 <div>
  <div class=t1>DeskBot Manual</div>
  <div class=t2>Direct firmware control · 192.168.4.1</div>
 </div>
</div>

<div class=card>
 <div class=row>
  <div><span class=dot></span>Connected</div>
  <span class=pill>Human Control</span>
 </div>
</div>

<div class=card>
 <p class=label>D-pad</p>
 <div class=dpad>
  <button class="btn spin" data-c=SPIN_LEFT>&#8630;</button>
  <button class=btn        data-c=FORWARD>&uarr;</button>
  <button class="btn spin" data-c=SPIN_RIGHT>&#8631;</button>
  <button class=btn        data-c=LEFT>&larr;</button>
  <button class="btn stop" data-c=STOP>STOP</button>
  <button class=btn        data-c=RIGHT>&rarr;</button>
  <button class="btn empty"></button>
  <button class=btn        data-c=BACKWARD>&darr;</button>
  <button class="btn empty"></button>
 </div>
</div>

<div class=card>
 <p class=label>Mode</p>
 <button class="btn primary" id=ai style="width:100%">&#8617; Switch back to AI</button>
</div>

<div class=foot>Hold a direction to keep moving · Space = STOP</div>
<div class=toast id=toast></div>

<script>
const T=(m,e)=>{const t=document.getElementById('toast');
 t.textContent=m;t.className='toast show'+(e?' error':'');
 clearTimeout(T._h);T._h=setTimeout(()=>t.className='toast',1500)};
function send(qs,ok){return fetch('/cmd?'+qs,{method:'POST'})
 .then(r=>r.ok?T(ok):T('Error',1)).catch(()=>T('Network error',1))}
function c(x){return send('m='+x,x)}
document.querySelectorAll('[data-c]').forEach(b=>{
 b.addEventListener('click',()=>c(b.dataset.c))});
document.getElementById('ai').addEventListener('click',()=>{
 send('mode=ai','Switching to AI mode...').then(()=>setTimeout(()=>{
  document.body.innerHTML='<div style="padding:40px;text-align:center;color:var(--text-mute)">Switched to AI mode. You can close this tab.</div>'},800))});
document.addEventListener('keydown',e=>{
 if(e.repeat||e.target.matches('input'))return;
 const M={ArrowUp:'FORWARD',ArrowDown:'BACKWARD',ArrowLeft:'LEFT',
  ArrowRight:'RIGHT',q:'SPIN_LEFT',Q:'SPIN_LEFT',e:'SPIN_RIGHT',
  E:'SPIN_RIGHT',' ':'STOP',Escape:'STOP'};
 const m=M[e.key];if(m){e.preventDefault();c(m)}});
</script>
</body></html>)HTML";

static MotionCmd Parse(const std::string& s) {
    if (s == "FORWARD")    return MotionCmd::FORWARD;
    if (s == "BACKWARD")   return MotionCmd::BACKWARD;
    if (s == "LEFT")       return MotionCmd::LEFT;
    if (s == "RIGHT")      return MotionCmd::RIGHT;
    if (s == "SPIN_LEFT")  return MotionCmd::SPIN_LEFT;
    if (s == "SPIN_RIGHT") return MotionCmd::SPIN_RIGHT;
    if (s == "STOP")       return MotionCmd::STOP;
    return MotionCmd::NONE;
}

esp_err_t WifiManualControl::HandleRoot(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, kIndexHtml, sizeof(kIndexHtml) - 1);
}

esp_err_t WifiManualControl::HandleCmd(httpd_req_t* req) {
    auto* self = static_cast<WifiManualControl*>(req->user_ctx);
    char qbuf[64] = {0};
    size_t qlen = httpd_req_get_url_query_len(req);
    if (qlen > 0 && qlen < sizeof(qbuf)) {
        httpd_req_get_url_query_str(req, qbuf, sizeof(qbuf));
    }
    char val[24] = {0};
    Command cmd = {};
    cmd.source = CommandSource::WIFI_MANUAL;

    if (httpd_query_key_value(qbuf, "m", val, sizeof(val)) == ESP_OK) {
        cmd.motion = Parse(val);
    } else if (httpd_query_key_value(qbuf, "mode", val, sizeof(val)) == ESP_OK) {
        cmd.mode = (std::strcmp(val, "ai") == 0) ? ModeCmd::SWITCH_AI
                                                  : ModeCmd::SWITCH_MANUAL;
    }
    if (self->sink_) self->sink_(cmd);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

esp_err_t WifiManualControl::HandleStatus(httpd_req_t* req) {
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"running\":true}");
}

esp_err_t WifiManualControl::RegisterHandlers() {
    httpd_uri_t u_root   = { "/",       HTTP_GET,  &HandleRoot,   this };
    httpd_uri_t u_cmd    = { "/cmd",    HTTP_POST, &HandleCmd,    this };
    httpd_uri_t u_status = { "/status", HTTP_GET,  &HandleStatus, this };
    esp_err_t e;
    if ((e = httpd_register_uri_handler(http_, &u_root))   != ESP_OK) return e;
    if ((e = httpd_register_uri_handler(http_, &u_cmd))    != ESP_OK) return e;
    if ((e = httpd_register_uri_handler(http_, &u_status)) != ESP_OK) return e;
    return ESP_OK;
}

bool WifiManualControl::Start() {
    if (running_) return true;

    // NOTE: Assumes TCP/IP + default event loop already initialized by the
    // existing WifiBoard. We just add an AP netif if one isn't active yet.
    static esp_netif_t* ap_netif = nullptr;
    if (ap_netif == nullptr) {
        ap_netif = esp_netif_create_default_wifi_ap();
    }

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    char ssid[32];
    int ssid_len = snprintf(ssid, sizeof(ssid), "DeskBot-%02X%02X", mac[4], mac[5]);

    wifi_config_t ap_cfg = {};
    std::memcpy(ap_cfg.ap.ssid, ssid, ssid_len);
    ap_cfg.ap.ssid_len     = ssid_len;
    ap_cfg.ap.channel      = AP_CHANNEL;
    ap_cfg.ap.max_connection = AP_MAX_CONN;
    ap_cfg.ap.authmode     = WIFI_AUTH_WPA2_PSK;
    std::memcpy(ap_cfg.ap.password, AP_PASS, std::strlen(AP_PASS));

    // Coexist with STA (used by xiaozhi for AI backend).
    wifi_mode_t cur = WIFI_MODE_NULL;
    esp_wifi_get_mode(&cur);
    wifi_mode_t want = (cur == WIFI_MODE_STA) ? WIFI_MODE_APSTA : WIFI_MODE_AP;
    ESP_ERROR_CHECK(esp_wifi_set_mode(want));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_LOGI(TAG, "SoftAP up: SSID='%s' pass='%s' 192.168.4.1", ssid, AP_PASS);

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true;
    cfg.stack_size = 4096;
    if (httpd_start(&http_, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        return false;
    }
    if (RegisterHandlers() != ESP_OK) {
        ESP_LOGE(TAG, "handler register failed");
        httpd_stop(http_); http_ = nullptr; return false;
    }
    running_ = true;
    return true;
}

void WifiManualControl::Stop() {
    if (!running_) return;
    if (http_) { httpd_stop(http_); http_ = nullptr; }
    running_ = false;
    ESP_LOGI(TAG, "manual HTTP stopped (SoftAP left up)");
}

}  // namespace robot
