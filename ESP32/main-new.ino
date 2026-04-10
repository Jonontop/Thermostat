#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// ── Pin definitions ────────────────────────────────────────────────
#define DHTPIN      15
#define DHTTYPE     DHT11
#define RELAY_PIN   5

// ── WiFi credentials ───────────────────────────────────────────────
const char* WIFI_SSID     = "SSID";
const char* WIFI_PASSWORD = "Password";

// ── Flask API base URL (no trailing slash) ─────────────────────────
// Change to your server's IP/hostname if not running on localhost
const char* API_BASE = "http:/api.pecar.site/api/state";  // <- update this

// ── Intervals (ms) ────────────────────────────────────────────────
const unsigned long DHT_INTERVAL      = 5000;   // interval branja senzorja
const unsigned long API_PULL_INTERVAL = 10000;  // fetch target + power state every 10 s
const unsigned long API_PUSH_INTERVAL = 10000;  // post temperature to API every 10 s

// ── LCD 16x2 I2C ──────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── DHT sensor ────────────────────────────────────────────────────
DHT dht(DHTPIN, DHTTYPE);

// ── Runtime state ─────────────────────────────────────────────────
float   currentTemperature  = 0.0;
float   targetTemperature   = 22.0;  // pulled from API
bool    thermostatOn        = true;  // pulled from API (is_on)
bool    relayState          = false;

unsigned long lastDHTRead   = 0;
unsigned long lastAPIPull   = 0;
unsigned long lastAPIPush   = 0;

// ── Custom LCD character: degree symbol ───────────────────────────
byte degreeChar[8] = {
  0b00110, 0b01001, 0b01001, 0b00110,
  0b00000, 0b00000, 0b00000, 0b00000
};

// ─────────────────────────────────────────────────────────────────
//  WiFi helpers
// ─────────────────────────────────────────────────────────────────
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi ");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    // Animate dots on row 1
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi connected! ");
    lcd.setCursor(0, 1);
    // Show last two octets of IP so it fits on 16 chars
    lcd.print(WiFi.localIP().toString().substring(0, 16));
    delay(2000);
  } else {
    Serial.println("\nWiFi FAILED — running offline");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi FAILED!    ");
    lcd.setCursor(0, 1);
    lcd.print("Offline mode    ");
    delay(2000);
  }
}

// ─────────────────────────────────────────────────────────────────
//  Relay logic
//  Simple hysteresis: turn on when temp drops 0.5 below target,
//  stay on until 0.5 above target.
// ─────────────────────────────────────────────────────────────────
void updateRelay() {
  bool newState = false;

  if (thermostatOn && !isnan(currentTemperature)) {
    if (!relayState && currentTemperature < targetTemperature - 0.5) {
      newState = true;   // kick on
    } else if (relayState && currentTemperature < targetTemperature + 0.5) {
      newState = true;   // keep on until we overshoot
    }
  }

  if (newState != relayState) {
    relayState = newState;
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    Serial.printf("[Relay] -> %s  (curr=%.1f  target=%.1f)\n",
                  relayState ? "ON" : "OFF", currentTemperature, targetTemperature);
  }
}

// ─────────────────────────────────────────────────────────────────
//  LCD updater
//
//  Row 0: "Now:21.3* T:22.0*"   (* = degree symbol, 16 chars)
//  Row 1: "Heating  [ON]   "
//         "Standby  [OFF]  "
//         " [THERMOSTAT OFF]"
// ─────────────────────────────────────────────────────────────────
void updateDisplay() {
  // ── Row 0 ─────────────────────────────────────────────────────
  lcd.setCursor(0, 0);
  lcd.print("Now:");

  if (isnan(currentTemperature)) {
    lcd.print("Err  ");
  } else {
    char buf[5];
    // 4 chars wide, 1 decimal: e.g. "21.3"
    dtostrf(currentTemperature, 4, 1, buf);
    lcd.print(buf);
    lcd.write(byte(0));  // degree char
  }

  lcd.print(" T:");
  char tbuf[5];
  dtostrf(targetTemperature, 4, 1, tbuf);
  lcd.print(tbuf);
  lcd.write(byte(0));   // degree char

  // ── Row 1 ─────────────────────────────────────────────────────
  lcd.setCursor(0, 1);
  if (!thermostatOn) {
    lcd.print("  [THERMOST OFF]");
  } else if (relayState) {
    lcd.print("Heating   [ON]  ");
  } else {
    lcd.print("Standby  [OFF]  ");
  }
}

// ─────────────────────────────────────────────────────────────────
//  API — GET /state  (pull target temp + is_on from Flask)
// ─────────────────────────────────────────────────────────────────
void pullAPIState() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(API_BASE) + "/state";
  http.begin(url);
  http.setTimeout(5000);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      targetTemperature = doc["temperature"].as<float>();
      thermostatOn      = doc["is_on"].as<bool>();
      Serial.printf("[API] Pulled — is_on=%d  target=%.1f\n",
                    (int)thermostatOn, targetTemperature);
    } else {
      Serial.println("[API] JSON parse error: " + String(err.c_str()));
    }
  } else {
    Serial.printf("[API] GET /state failed: HTTP %d\n", code);
  }

  http.end();
}

// ─────────────────────────────────────────────────────────────────
//  API — POST /state  (push sensor reading to Flask log)
// ─────────────────────────────────────────────────────────────────
void pushTemperature() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (isnan(currentTemperature))       return;

  HTTPClient http;
  String url = String(API_BASE) + "/state";
  http.begin(url);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["temperature"] = currentTemperature;
  doc["is_on"]       = thermostatOn && relayState;  // relay reflects real activity

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code == 200) {
    Serial.printf("[API] Pushed temp=%.1f  relay=%d  HTTP %d\n",
                  currentTemperature, (int)relayState, code);
  } else {
    Serial.printf("[API] POST /state failed: HTTP %d\n", code);
  }

  http.end();
}

// ─────────────────────────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Relay — safe default
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // LCD initialise
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degreeChar);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Thermostat v2  ");
  lcd.setCursor(0, 1);
  lcd.print("  Starting...   ");
  delay(1200);

  // DHT sensor
  dht.begin();

  // WiFi
  connectWiFi();

  // Grab initial state from API
  pullAPIState();

  // Show initial display
  updateDisplay();
}

// ─────────────────────────────────────────────────────────────────
//  Loop
// ─────────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── Read DHT11 ────────────────────────────────────────────────
  if (now - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = now;

    float temp = dht.readTemperature();
    if (!isnan(temp)) {
      currentTemperature = temp;
      Serial.printf("[DHT] %.1f°C\n", currentTemperature);
    } else {
      Serial.println("[DHT] Read failed — keeping last value");
    }

    updateRelay();
    updateDisplay();
  }

  // ── Pull state from Flask (target temp + on/off) ───────────────
  if (now - lastAPIPull >= API_PULL_INTERVAL) {
    lastAPIPull = now;
    pullAPIState();
    updateRelay();    // re-evaluate after new target
    updateDisplay();
  }

  // ── Push current temp + relay state to Flask ──────────────────
  if (now - lastAPIPush >= API_PUSH_INTERVAL) {
    lastAPIPush = now;
    pushTemperature();
  }

  // ── WiFi watchdog ──────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Lost — reconnecting…");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi lost...    ");
    connectWiFi();
  }
}
