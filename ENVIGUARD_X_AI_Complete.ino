/*
 ENVIGUARD-X AI - COMPLETE ESP32 PHASE 1
 ESP32 Dev Module / DHT22 / SSD1306 OLED / 3 LEDs / active buzzer
 Change WIFI_SSID and WIFI_PASSWORD below.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

const char* WIFI_SSID = "GUGANRAJ";
const char* WIFI_PASSWORD = "12345678";

constexpr uint8_t DHT_PIN = 4;
#define DHT_TYPE DHT22
constexpr uint8_t OLED_SDA = 21, OLED_SCL = 22;
constexpr uint8_t GREEN_LED = 27, BLUE_LED = 26, RED_LED = 14;
constexpr uint8_t BUZZER_PIN = 25;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr float MEDIUM_TEMP = 30.0f;
constexpr float HIGH_TEMP = 40.0f;
constexpr unsigned long READ_INTERVAL = 2500UL;
constexpr unsigned long OLED_PAGE_INTERVAL = 3000UL;
constexpr unsigned long WIFI_RETRY_INTERVAL = 15000UL;

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WebServer server(80);
bool oledReady = false, sensorOK = false, alertActive = false;
float temperature = NAN, humidity = NAN;
float minTemperature = NAN, maxTemperature = NAN;
float minHumidity = NAN, maxHumidity = NAN;
unsigned long lastRead = 0, lastWiFiRetry = 0, lastOLEDRefresh = 0;
bool previouslyConnected = false;

String levelName() {
  if (!sensorOK) return "SENSOR ERROR";
  if (temperature >= HIGH_TEMP) return "HIGH";
  if (temperature >= MEDIUM_TEMP) return "MEDIUM";
  return "LOW";
}

void updateOutputs() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  alertActive = false;
  if (!sensorOK) return;
  if (temperature >= HIGH_TEMP) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    alertActive = true;
  } else if (temperature >= MEDIUM_TEMP) {
    digitalWrite(BLUE_LED, HIGH);
  } else {
    digitalWrite(GREEN_LED, HIGH);
  }
}

void readSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(t) || isnan(h) || t < -40 || t > 80 || h < 0 || h > 100) {
    sensorOK = false;
    temperature = NAN;
    humidity = NAN;
    updateOutputs();
    Serial.println("DHT22 SENSOR ERROR: Check wiring and pull-up resistor.");
    return;
  }
  sensorOK = true;
  temperature = t;
  humidity = h;
  if (isnan(minTemperature) || t < minTemperature) minTemperature = t;
  if (isnan(maxTemperature) || t > maxTemperature) maxTemperature = t;
  if (isnan(minHumidity) || h < minHumidity) minHumidity = h;
  if (isnan(maxHumidity) || h > maxHumidity) maxHumidity = h;
  updateOutputs();
  Serial.printf("Temp: %.1f C | Humidity: %.1f %% | Level: %s | Buzzer: %s\n",
                t, h, levelName().c_str(), alertActive ? "ON" : "OFF");
}

void updateOLED() {
  if (!oledReady) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  // Prioritize faults and critical alarms instead of waiting for page rotation.
  if (!sensorOK) {
    display.setTextSize(1);
    display.setCursor(0, 0); display.println("ENVIGUARD-X AI");
    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 20); display.println("SENSOR");
    display.setCursor(0, 40); display.println("ERROR!");
  } else if (alertActive) {
    display.setTextSize(1);
    display.setCursor(0, 0); display.println("CRITICAL WARNING!");
    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);
    display.setTextSize(3);
    display.setCursor(28, 18); display.println("HIGH");
    display.setTextSize(1);
    display.setCursor(13, 53); display.println("RED + BUZZER ON");
  } else {
    const uint8_t page = (millis() / OLED_PAGE_INTERVAL) % 4;
    display.setTextSize(1);
    display.setCursor(0, 0); display.println("ENVIGUARD-X AI");
    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);
    if (page == 0) {
      display.setCursor(0, 17); display.println("TEMPERATURE");
      display.setTextSize(2);
      display.setCursor(0, 32);
      display.print(temperature, 1); display.print(" C");
    } else if (page == 1) {
      display.setCursor(0, 17); display.println("HUMIDITY");
      display.setTextSize(2);
      display.setCursor(0, 32);
      display.print(humidity, 1); display.print("%");
    } else if (page == 2) {
      display.setCursor(0, 17); display.println("WARNING LEVEL");
      display.setTextSize(3);
      display.setCursor(0, 31); display.print(levelName());
    } else {
      display.setCursor(0, 17); display.println("BUZZER / NETWORK");
      display.setTextSize(2);
      display.setCursor(0, 30); display.println("OFF");
      display.setTextSize(1);
      display.setCursor(0, 53);
      display.print("WiFi: ");
      display.print(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "OFFLINE");
    }
    if (page != 3) {
      display.setTextSize(1);
      display.setCursor(104, 56);
      display.print(page + 1); display.print("/4");
    }
  }
  display.display();
}

String numberOrNull(float v) { return isnan(v) ? "null" : String(v, 1); }
void handleData() {
  String json;
  json.reserve(260);
  json = "{\"temperature\":" + numberOrNull(temperature);
  json += ",\"humidity\":" + numberOrNull(humidity);
  json += ",\"minTemp\":" + numberOrNull(minTemperature);
  json += ",\"maxTemp\":" + numberOrNull(maxTemperature);
  json += ",\"minHum\":" + numberOrNull(minHumidity);
  json += ",\"maxHum\":" + numberOrNull(maxHumidity);
  json += ",\"sensorOK\":";
  json += sensorOK ? "true" : "false";
  json += ",\"alert\":";
  json += alertActive ? "true" : "false";
  json += ",\"comfort\":\"" + levelName() + "\"";
  json += ",\"uptime\":" + String(millis() / 1000UL);
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// Replace the previous DASHBOARD definition with this header.
const char DASHBOARD[] PROGMEM = R"ENVX(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ENVIGUARD-X AI | Mission Control</title>
<style>
:root{--bg:#070d1c;--panel:#111d34;--panel2:#192a47;--line:#2c405f;--ink:#f4f8ff;--muted:#9aacc7;--aqua:#4adcf5;--green:#32df8a;--blue:#478bff;--red:#ff5269;--amber:#ffcc65}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:14px system-ui,Segoe UI,Arial,sans-serif}body.light{--bg:#eaf1fa;--panel:#fff;--panel2:#edf3fa;--line:#ccd8e8;--ink:#15243d;--muted:#64748b}
body:before{content:"";position:fixed;inset:0;pointer-events:none;background:radial-gradient(circle at 15% 5%,#0b8ecb22,transparent 35%),radial-gradient(circle at 85% 55%,#4455d011,transparent 35%)}.wrap{position:relative;max-width:1440px;margin:auto;padding:22px}
header,.between{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap}header{margin-bottom:18px}.brand{font-size:25px;font-weight:900;letter-spacing:1px}.brand em{font-style:normal;color:var(--aqua)}.eyebrow{color:var(--muted);font-size:11px;letter-spacing:2px;margin-top:5px}.actions{display:flex;gap:8px;align-items:center;flex-wrap:wrap}button{background:var(--panel2);border:1px solid var(--line);color:var(--ink);padding:10px 13px;border-radius:10px;cursor:pointer;font-weight:700}button:hover{border-color:var(--aqua)}button:focus-visible{outline:2px solid var(--aqua)}.chip{padding:9px 12px;border:1px solid var(--line);border-radius:999px;background:var(--panel);font-size:12px;font-weight:800}.pulse{display:inline-block;width:8px;height:8px;background:var(--amber);border-radius:50%;margin-right:7px;animation:pulse 1.6s infinite}@keyframes pulse{50%{box-shadow:0 0 0 7px #32df8a22}}
.grid{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:14px}.card{background:var(--panel);border:1px solid var(--line);border-radius:18px;padding:18px;min-width:0;box-shadow:0 12px 28px #00000012}.span3{grid-column:span 3}.span4{grid-column:span 4}.span5{grid-column:span 5}.span6{grid-column:span 6}.span7{grid-column:span 7}.span8{grid-column:span 8}.span12{grid-column:span 12}
.label{color:var(--muted);font-size:11px;letter-spacing:1.4px;font-weight:800;text-transform:uppercase}.reading{font-size:clamp(27px,3vw,43px);font-weight:900;margin:12px 0 6px;letter-spacing:-1px}.muted{color:var(--muted)}.aqua{color:var(--aqua)}.green{color:var(--green)}.red{color:var(--red)}.blue{color:var(--blue)}h2{font-size:16px;margin:0 0 14px}h3{font-size:13px;margin:0 0 10px}.mini{font-size:12px}.metric{display:flex;justify-content:space-between;gap:10px;padding:10px 0;border-bottom:1px solid var(--line)}.metric:last-child{border:0}.metric b{font-variant-numeric:tabular-nums}
.hero{position:relative;overflow:hidden;background:linear-gradient(120deg,var(--panel),var(--panel2));}.hero:after{content:"";position:absolute;width:190px;height:190px;border:1px solid #4adcf522;border-radius:50%;right:-70px;top:-90px;box-shadow:0 0 0 28px #4adcf50a,0 0 0 56px #4adcf508;pointer-events:none}.hero strong{font-size:22px}.hero p{color:var(--muted);max-width:650px;line-height:1.6}.alertBanner{display:none;border:1px solid #ff5269;background:#6f1d32;color:white;border-radius:14px;padding:13px 18px;font-weight:800;margin-bottom:14px;animation:alertGlow 1.6s infinite}@keyframes alertGlow{50%{box-shadow:0 0 22px #ff526950}}.alertBanner.sensor{background:#765019;border-color:#ffcc65;animation:none}
.lights{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.lightBox{text-align:center;border:1px solid var(--line);background:var(--panel2);border-radius:14px;padding:14px 5px;opacity:.38;transition:transform .3s,opacity .3s,border-color .3s}.lightBox.on{opacity:1;transform:translateY(-4px);border-color:currentColor}.lightBox.low{color:var(--green)}.lightBox.medium{color:var(--blue)}.lightBox.high{color:var(--red)}.bulb{height:43px;width:43px;border-radius:50%;background:currentColor;margin:0 auto 10px}.on .bulb{animation:bulbGlow 1.8s infinite alternate}@keyframes bulbGlow{to{box-shadow:0 0 22px currentColor,0 0 42px currentColor}}.lightBox b{display:block;font-size:12px}.lightBox small{display:block;color:var(--muted);margin-top:5px;font-size:10px}
.gauge{--angle:0deg;--gaugeColor:var(--aqua);width:176px;height:176px;margin:6px auto 10px;border-radius:50%;display:grid;place-items:center;background:conic-gradient(var(--gaugeColor) var(--angle),var(--line) 0);transition:background .3s}.gaugeInside{width:142px;height:142px;border-radius:50%;background:var(--panel);display:flex;flex-direction:column;align-items:center;justify-content:center}.gaugeInside strong{font-size:30px}.gaugeInside span{color:var(--muted);font-size:10px}
.bar{height:8px;border-radius:10px;background:var(--panel2);overflow:hidden;margin:12px 0}.bar>div{height:100%;width:0;background:var(--aqua);border-radius:10px;transition:width .7s}.chart{display:block;width:100%;height:180px}.events{max-height:250px;overflow:auto}.event{padding:10px 0;border-bottom:1px solid var(--line);font-size:12px;line-height:1.5}.event time{color:var(--aqua);font-weight:800;margin-right:8px}.event:last-child{border:0}.tag{display:inline-block;padding:6px 9px;border-radius:8px;background:var(--panel2);font-size:12px;font-weight:800}.footer{text-align:center;color:var(--muted);font-size:11px;margin:25px 0 5px}.ticker{font-size:12px;color:var(--muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.fade{animation:rise .6s both}@keyframes rise{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}@media(max-width:1000px){.span3{grid-column:span 6}.span4,.span5,.span7,.span8{grid-column:span 6}}@media(max-width:650px){.wrap{padding:12px}.span3,.span4,.span5,.span6,.span7,.span8{grid-column:span 12}.brand{font-size:21px}.card{padding:15px}.actions{width:100%}.lights{gap:6px}}
@media(prefers-reduced-motion:reduce){*,*:before,*:after{animation:none!important;transition:none!important}}
</style></head><body><div class="wrap">
<header><div><div class="brand">◉ ENVIGUARD-<em>Device</em></div><div class="eyebrow">ENVIRONMENTAL INTELLIGENCE · MISSION CONTROL</div></div><div class="actions"><span class="chip"><i class="pulse" id="dot"></i><span id="connection">CONNECTING</span></span><button id="themeBtn">☀ Theme</button><button id="fullBtn">⛶ Expo Mode</button><button id="exportBtn">↓ CSV</button></div></header>
<div class="alertBanner" id="critical">⚠ CRITICAL TEMPERATURE — RED LED & PHYSICAL BUZZER ACTIVE</div>
<div class="alertBanner sensor" id="sensorError">⚠ DHT22 SENSOR ERROR — CHECK CONNECTIONS</div>
<div class="grid">
<section class="card hero span12 fade"><div class="between"><div><div class="label">LIVE ENVIRONMENTAL OVERVIEW</div><p><strong id="heroTitle">Awaiting first sensor reading</strong></p><p id="heroDesc">Connect your ESP32 and keep this page open to begin real-time monitoring.</p></div><span class="chip" id="heroLevel">AWAITING DATA</span></div><div class="ticker" id="ticker">● ESP32 / DHT22 / Three LED warning / Local dashboard / No cloud required</div></section>
<section class="card span3 fade"><div class="label">Temperature</div><div class="reading aqua" id="temp">--</div><div class="mini muted">Live DHT22 reading</div></section>
<section class="card span3 fade"><div class="label">Humidity</div><div class="reading green" id="hum">--</div><div class="mini muted">Relative humidity</div></section>
<section class="card span3 fade"><div class="label">Temperature trend</div><div class="reading" id="trend" style="font-size:25px">--</div><div class="mini muted" id="trendDetail">Collecting samples...</div></section>
<section class="card span3 fade"><div class="label">Critical buzzer</div><div class="reading green" id="buzzer">--</div><div class="mini muted">Physical output · GPIO 25</div></section>
<section class="card span7 fade"><h2>🚦 Three-Level Warning System</h2><div class="lights"><div class="lightBox low" id="low"><div class="bulb"></div><b>LOW</b><small>Below 30°C</small></div><div class="lightBox medium" id="medium"><div class="bulb"></div><b>MEDIUM</b><small>30–39.9°C</small></div><div class="lightBox high" id="high"><div class="bulb"></div><b>HIGH</b><small>40°C and above</small></div></div><div class="metric" style="margin-top:13px"><span class="muted">Current hardware indicator</span><b id="indicator">--</b></div><div class="metric"><span class="muted">Alarm rule</span><b>HIGH level only</b></div></section>
<section class="card span5 fade"><h2>◉ Animated Temperature Gauge</h2><div class="gauge" id="gauge"><div class="gaugeInside"><strong id="gaugeText">--</strong><span>0–50°C DISPLAY SCALE</span></div></div><div class="between mini muted"><span>0°C</span><span>30°C medium</span><span>40°C high</span><span>50°C</span></div></section>
<section class="card span6 fade"><div class="between"><h2>↗ Temperature Waveform</h2><span class="tag" id="tempCount">0 points</span></div><canvas class="chart" id="tempChart"></canvas><div class="mini muted">Latest 60 valid browser samples</div></section>
<section class="card span6 fade"><div class="between"><h2>≈ Humidity Waveform</h2><span class="tag" id="humCount">0 points</span></div><canvas class="chart" id="humChart"></canvas><div class="mini muted">Updates approximately every 2.5 seconds</div></section>
<section class="card span4 fade"><h2>📊 Temperature Analytics</h2><div class="metric"><span class="muted">Minimum since boot</span><b id="minT">--</b></div><div class="metric"><span class="muted">Maximum since boot</span><b id="maxT">--</b></div><div class="metric"><span class="muted">Browser average</span><b id="avgT">--</b></div><div class="metric"><span class="muted">Last 4-interval change</span><b id="delta">--</b></div></section>
<section class="card span4 fade"><h2>💧 Humidity Analytics</h2><div class="metric"><span class="muted">Minimum since boot</span><b id="minH">--</b></div><div class="metric"><span class="muted">Maximum since boot</span><b id="maxH">--</b></div><div class="metric"><span class="muted">Browser average</span><b id="avgH">--</b></div><div class="metric"><span class="muted">Humidity assessment</span><b id="humAssessment">--</b></div></section>
<section class="card span4 fade"><h2>🩺 Device Health</h2><div class="metric"><span class="muted">Sensor</span><b id="health">--</b></div><div class="metric"><span class="muted">Device uptime</span><b id="uptime">--</b></div><div class="metric"><span class="muted">Browser samples</span><b id="samples">0</b></div><div class="metric"><span class="muted">Last response</span><b id="lastSeen">--</b></div><div class="label" style="margin-top:15px">Browser session capacity</div><div class="bar"><div id="capacity"></div></div><div class="mini muted">Up to 5,000 samples in browser memory</div></section>
<section class="card span6 fade"><h2>🧠 Smart Environmental Assessment</h2><div class="reading" id="assessment" style="font-size:22px">WAITING FOR DATA</div><p class="muted" id="advice" style="line-height:1.7">Rule-based environmental assessment will appear here.</p><div class="tag">RULE-BASED ANALYTICS · NOT TRAINED AI</div></section>
<section class="card span6 fade"><div class="between"><h2>🕒 Live Event Timeline</h2><button id="clearEvents">Clear events</button></div><div class="events" id="events"><div class="event muted">Waiting for device...</div></div></section>
<section class="card span12 fade"><div class="between"><div><h2>📁 Exhibition Data Center</h2><div class="mini muted">Download your real sensor observations as CSV. History is temporary and is not uploaded to a server.</div></div><div class="actions"><button id="exportBtn2">↓ Export CSV</button><button id="clearData">Clear browser data</button></div></div></section>
</div><div class="footer">ENVIGUARD-X AI · ESP32-WROOM-32 + DHT22 · LOCAL COLLEGE EXPO PROTOTYPE · NO PAID SERVICES</div>
</div>
<script>
const $=id=>document.getElementById(id);
const MAX_RECORDS=5000,MAX_GRAPH=60;
let records=[],tHist=[],hHist=[],events=[],previousLevel="",previousValid=null,lastUptime=null,connected=false,busy=false,lastSampleUptime=null;
function fmt(v,u){return typeof v==="number"&&Number.isFinite(v)?v.toFixed(1)+u:"--"}
function logEvent(message){events.unshift({time:new Date().toLocaleTimeString(),message});events=events.slice(0,16);$("events").replaceChildren(...events.map(e=>{const d=document.createElement("div");d.className="event";const t=document.createElement("time");t.textContent=e.time;d.append(t,document.createTextNode(e.message));return d}))}
function levelUI(level){for(const x of ["low","medium","high"])$(x).classList.toggle("on",x===level.toLowerCase());$("heroLevel").textContent=level==="ERROR"?"SENSOR ERROR":level+" LEVEL";$("heroLevel").style.color=level==="HIGH"?"var(--red)":level==="MEDIUM"?"var(--blue)":level==="LOW"?"var(--green)":"var(--amber)";$("indicator").textContent=level==="ERROR"?"ALL OFF":level+" LED"}
function gauge(t){if(t===null){$("gaugeText").textContent="--";$("gauge").style.setProperty("--angle","0deg");return}const color=t>=40?"var(--red)":t>=30?"var(--blue)":"var(--green)";$("gauge").style.setProperty("--gaugeColor",color);$("gauge").style.setProperty("--angle",(Math.max(0,Math.min(50,t))/50*360)+"deg");$("gaugeText").textContent=t.toFixed(1)+"°C"}
function chart(id,values,color){const c=$(id),ctx=c.getContext("2d"),w=c.clientWidth,h=180,dpr=devicePixelRatio||1;c.width=Math.round(w*dpr);c.height=Math.round(h*dpr);ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,w,h);if(!values.length){ctx.fillStyle="#9aacc7";ctx.font="12px Arial";ctx.fillText("Waiting for valid samples...",12,90);return}let lo=Math.min(...values),hi=Math.max(...values);if(hi-lo<2){lo-=1;hi+=1}else{const p=(hi-lo)*.15;lo-=p;hi+=p}const l=42,r=Math.max(l+10,w-10),top=12,bottom=150;ctx.font="11px Arial";for(let i=0;i<=4;i++){const y=top+(bottom-top)*i/4;ctx.strokeStyle="#415575";ctx.beginPath();ctx.moveTo(l,y);ctx.lineTo(r,y);ctx.stroke();ctx.fillStyle="#9aacc7";ctx.fillText((hi-(hi-lo)*i/4).toFixed(1),2,y+4)}ctx.strokeStyle=color;ctx.lineWidth=2.6;ctx.beginPath();values.forEach((v,i)=>{const x=values.length===1?l:l+(r-l)*i/(values.length-1),y=bottom-(v-lo)/(hi-lo)*(bottom-top);if(!i)ctx.moveTo(x,y);else ctx.lineTo(x,y)});ctx.stroke();const v=values[values.length-1],x=values.length===1?l:r,y=bottom-(v-lo)/(hi-lo)*(bottom-top);ctx.beginPath();ctx.arc(x,y,4,0,Math.PI*2);ctx.fillStyle=color;ctx.fill()}
function redraw(){chart("tempChart",tHist,"#4adcf5");chart("humChart",hHist,"#32df8a");$("tempCount").textContent=tHist.length+" points";$("humCount").textContent=hHist.length+" points"}
function averages(){if(!records.length){for(const id of ["avgT","avgH"])$(id).textContent="--";$("samples").textContent="0";$("capacity").style.width="0%";return}const sum=i=>records.reduce((a,r)=>a+r[i],0)/records.length;$("avgT").textContent=fmt(sum(1),"°C");$("avgH").textContent=fmt(sum(2),"%");$("samples").textContent=records.length;$("capacity").style.width=(records.length/MAX_RECORDS*100)+"%"}
function trend(){if(tHist.length<5){$("trend").textContent="COLLECTING";$("trend").style.color="var(--muted)";$("trendDetail").textContent="Need 5 samples";$("delta").textContent="--";return}const d=tHist[tHist.length-1]-tHist[tHist.length-5];$("delta").textContent=(d>0?"+":"")+d.toFixed(1)+"°C";const up=d>.4,down=d<-.4;$("trend").textContent=up?"↗ RISING":down?"↘ FALLING":"→ STABLE";$("trend").style.color=up?"var(--red)":down?"var(--aqua)":"var(--green)";$("trendDetail").textContent="Across four sampling intervals"}
function disconnected(){if(connected)logEvent("Dashboard connection lost");connected=false;$("connection").textContent="DISCONNECTED";$("dot").style.background="var(--red)";$("health").textContent="UNKNOWN";$("buzzer").textContent="UNKNOWN";$("buzzer").className="reading muted";$("critical").style.display="none";$("heroTitle").textContent="ESP32 connection lost";$("heroDesc").textContent="Check the ESP32 Wi-Fi and ensure your phone or laptop is on the same network."}
async function refresh(){if(busy)return;busy=true;try{const res=await fetch("/data",{cache:"no-store"});if(!res.ok)throw Error("HTTP "+res.status);const d=await res.json();if(!connected)logEvent("Dashboard connected to ESP32");connected=true;$("connection").textContent="LIVE CONNECTED";$("dot").style.background="var(--green)";$("lastSeen").textContent=new Date().toLocaleTimeString();$("uptime").textContent=Math.floor(d.uptime/60)+"m "+d.uptime%60+"s";$("temp").textContent=fmt(d.temperature,"°C");$("hum").textContent=fmt(d.humidity,"%");$("minT").textContent=fmt(d.minTemp,"°C");$("maxT").textContent=fmt(d.maxTemp,"°C");$("minH").textContent=fmt(d.minHum,"%");$("maxH").textContent=fmt(d.maxHum,"%");
if(lastUptime!==null&&d.uptime<lastUptime){logEvent("ESP32 restarted");previousLevel="";lastSampleUptime=null}lastUptime=d.uptime;
const valid=d.sensorOK&&typeof d.temperature==="number"&&typeof d.humidity==="number";if(previousValid!==null&&previousValid!==valid)logEvent(valid?"DHT22 recovered":"DHT22 reading failed");previousValid=valid;$("health").textContent=valid?"HEALTHY":"SENSOR ERROR";$("sensorError").style.display=valid?"none":"block";
if(!valid){levelUI("ERROR");gauge(null);$("critical").style.display="none";$("buzzer").textContent="OFF";$("buzzer").className="reading green";$("heroTitle").textContent="Sensor reading unavailable";$("heroDesc").textContent="Inspect the DHT22 wiring and pull-up resistor.";$("assessment").textContent="SENSOR ERROR";$("advice").textContent="No valid reading is available. All three indicator LEDs and the buzzer should be off.";return}
const t=d.temperature,h=d.humidity,level=t>=40?"HIGH":t>=30?"MEDIUM":"LOW";levelUI(level);gauge(t);if(level!==previousLevel){logEvent("Warning changed to "+level+" at "+t.toFixed(1)+"°C");previousLevel=level}
$("critical").style.display=d.alert?"block":"none";$("buzzer").textContent=d.alert?"ON · ALERT":"OFF";$("buzzer").className="reading "+(d.alert?"red":"green");$("heroTitle").textContent=level==="HIGH"?"Critical temperature detected":level==="MEDIUM"?"Elevated temperature detected":"Environment within LOW temperature level";$("heroDesc").textContent=level==="HIGH"?"Red LED and buzzer should be active. Check the monitored area.":level==="MEDIUM"?"Blue LED active. Continue observing the rising or falling trend.":"Green LED active. Temperature is below the medium threshold.";
$("humAssessment").textContent=h>70?"HIGH":h<30?"LOW":"MODERATE";$("assessment").textContent=level==="HIGH"?"CRITICAL WARNING":level==="MEDIUM"?"CAUTION ADVISED":"NORMAL TEMPERATURE";$("assessment").style.color=level==="HIGH"?"var(--red)":level==="MEDIUM"?"var(--blue)":"var(--green)";$("advice").textContent=(level==="HIGH"?"High temperature threshold reached. Investigate the surroundings.":level==="MEDIUM"?"Temperature is elevated. Observe the trend and surroundings.":"Temperature is below the configured warning threshold.")+(h>70?" Humidity is above 70%.":h<30?" Humidity is below 30%.":" Humidity is between 30% and 70%.");
if(lastSampleUptime!==d.uptime){lastSampleUptime=d.uptime;tHist.push(t);hHist.push(h);if(tHist.length>MAX_GRAPH)tHist.shift();if(hHist.length>MAX_GRAPH)hHist.shift();records.push([new Date().toISOString(),t,h,level,d.alert?"ON":"OFF"]);if(records.length>MAX_RECORDS)records.shift();averages();trend();redraw()}
}catch(e){disconnected()}finally{busy=false}}
function csv(){const rows=[["Timestamp","Temperature_C","Humidity_Percent","Level","Buzzer"],...records];const content=rows.map(r=>r.join(",")).join("\r\n");const url=URL.createObjectURL(new Blob([content],{type:"text/csv;charset=utf-8"}));const a=document.createElement("a");a.href=url;a.download="ENVIGUARD_X_Expo_Data.csv";document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000)}
function clearData(){records=[];tHist=[];hHist=[];lastSampleUptime=null;averages();trend();redraw();logEvent("Browser sample history cleared")}
$("exportBtn").onclick=csv;$("exportBtn2").onclick=csv;$("clearData").onclick=clearData;$("clearEvents").onclick=()=>{events=[];$("events").textContent="Timeline cleared"};$("themeBtn").onclick=()=>document.body.classList.toggle("light");$("fullBtn").onclick=()=>{if(!document.fullscreenElement){document.documentElement.requestFullscreen?.()}else document.exitFullscreen?.()};window.addEventListener("resize",redraw);redraw();refresh();setInterval(refresh,2500);
</script></body></html>)ENVX";

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  updateOutputs();

  dht.begin();
  Wire.begin(OLED_SDA, OLED_SCL);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!oledReady) Serial.println("OLED not found (expected address 0x3C).");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to 2.4 GHz Wi-Fi...");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Open dashboard: http://");
    Serial.println(WiFi.localIP());
    previouslyConnected = true;
  } else {
    Serial.println("Wi-Fi unavailable. OLED and local LEDs still work.");
  }

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", DASHBOARD);
  });
  server.on("/data", HTTP_GET, handleData);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();

  // DHT22 needs at least two seconds between measurements.
  delay(2500);
  readSensor();
  updateOLED();
  lastRead = millis();
  lastWiFiRetry = millis();
  Serial.println("ENVIGUARD-X AI READY");
}

void loop() {
  server.handleClient();
  unsigned long now = millis();
  if (now - lastRead >= READ_INTERVAL) {
    lastRead = now;
    readSensor();
  }
  if (now - lastOLEDRefresh >= 250UL) {
    lastOLEDRefresh = now;
    updateOLED();
  }
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !previouslyConnected) {
    Serial.print("Dashboard reconnected: http://");
    Serial.println(WiFi.localIP());
    updateOLED();
  }
  previouslyConnected = connected;
  if (!connected && now - lastWiFiRetry >= WIFI_RETRY_INTERVAL) {
    lastWiFiRetry = now;
    WiFi.reconnect();
  }
}
