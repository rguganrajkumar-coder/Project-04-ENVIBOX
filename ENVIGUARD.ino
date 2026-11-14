#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =============================
// WIFI SETTINGS
// =============================
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// =============================
// DHT SENSOR
// =============================
#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// =============================
// OLED DISPLAY
// =============================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =============================
// WEB SERVER
// =============================
WebServer server(80);

float temperature = NAN;
float humidity = NAN;

unsigned long lastRead = 0;
const unsigned long readInterval = 2000;

// =============================
// HTML WEBPAGE
// =============================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
<meta charset="UTF-8">
<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>ESP32 Smart Temperature</title>

<style>
* {
  box-sizing: border-box;
}

body {
  margin: 0;
  padding: 20px;
  min-height: 100vh;
  font-family: Arial, sans-serif;
  background: linear-gradient(135deg,#0f172a,#1e3a5f);
  color: white;
  text-align: center;
}

.container {
  max-width: 550px;
  margin: 40px auto;
}

h1 {
  font-size: 27px;
}

.subtitle {
  color: #94a3b8;
  margin-bottom: 30px;
}

.card {
  background: #1e293b;
  padding: 30px;
  margin: 20px 0;
  border-radius: 18px;
  box-shadow: 0 8px 25px #0003;
}

.label {
  font-size: 19px;
  color: #cbd5e1;
}

.value {
  font-size: 52px;
  font-weight: bold;
  margin-top: 12px;
}

.temperature {
  color: #38bdf8;
}

.humidity {
  color: #4ade80;
}

.status {
  margin-top: 25px;
  color: #4ade80;
}

footer {
  color: #94a3b8;
  margin-top: 35px;
  font-size: 13px;
}
</style>
</head>

<body>

<div class="container">

<h1>Smart Temperature Monitor</h1>

<p class="subtitle">
ESP32 | DHT | OLED | IoT
</p>

<div class="card">
  <div class="label">Temperature</div>
  <div class="value temperature" id="temp">-- °C</div>
</div>

<div class="card">
  <div class="label">Humidity</div>
  <div class="value humidity" id="hum">-- %</div>
</div>

<div class="status" id="status">
Connecting...
</div>

<footer>
3D Printed Smart Temperature Display
</footer>

</div>

<script>

async function updateData() {
  try {
    const response = await fetch('/data', {
      cache: 'no-store'
    });

    if (!response.ok) {
      throw new Error('Server error');
    }

    const data = await response.json();

    document.getElementById("temp").innerText =
      data.temperature === null
      ? "-- °C"
      : data.temperature.toFixed(1) + " °C";

    document.getElementById("hum").innerText =
      data.humidity === null
      ? "-- %"
      : data.humidity.toFixed(1) + " %";

    document.getElementById("status").innerText =
      data.temperature === null || data.humidity === null
      ? "Waiting for sensor readings"
      : "● Live Monitoring";

  } catch (error) {
    document.getElementById("status").innerText =
      "Connection Lost";
  }
}

updateData();

setInterval(updateData, 2000);

</script>

</body>
</html>
)rawliteral";

// =============================
// READ SENSOR
// =============================
void readSensor() {

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    temperature = NAN;
    humidity = NAN;
    Serial.println("DHT reading failed!");
    return;
  }

  temperature = t;
  humidity = h;

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

// =============================
// UPDATE OLED
// =============================
void updateOLED() {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART TEMP MONITOR");

  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display.setCursor(0, 18);
  display.println("Temperature:");

  display.setTextSize(2);
  display.setCursor(0, 29);

  if (isnan(temperature)) {
    display.print("--");
  } else {
    display.print(temperature, 1);
  }

  display.print(" C");

  display.setTextSize(1);
  display.setCursor(0, 54);

  display.print("Humidity: ");

  if (isnan(humidity)) {
    display.print("--");
  } else {
    display.print(humidity, 1);
  }

  display.print(" %");

  display.display();
}

// =============================
// JSON API
// =============================
void handleData() {

  String json = "{";

  json += "\"temperature\":";
  json += isnan(temperature)
    ? "null" : String(temperature, 1);

  json += ",\"humidity\":";
  json += isnan(humidity)
    ? "null" : String(humidity, 1);

  json += "}";

  server.send(200, "application/json", json);
}

// =============================
// WIFI CONNECTION
// =============================
void connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi Connected!");

    Serial.print("Open Webpage: http://");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("WiFi connection failed!");
    Serial.println("Check WiFi name and password.");

  }
}

// =============================
// SETUP
// =============================
void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting Smart Temperature Monitor");

  dht.begin();

  Wire.begin(21, 22);

  if (!display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS)) {

    Serial.println("OLED initialization failed!");

    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("SMART TEMP DISPLAY");
  display.println("Starting...");

  display.display();

  readSensor();
  updateOLED();

  connectWiFi();

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/data", HTTP_GET, handleData);

  server.begin();

  Serial.println("Web Server Started");

  lastRead = millis();
}

// =============================
// MAIN LOOP
// =============================
void loop() {

  server.handleClient();

  unsigned long currentMillis = millis();

  if (currentMillis - lastRead >= readInterval) {

    lastRead = currentMillis;

    readSensor();
    updateOLED();
  }

  // Reconnect WiFi if disconnected
  static unsigned long lastReconnect = 0;

  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastReconnect >= 10000) {

    lastReconnect = millis();
    WiFi.reconnect();
  }
}



