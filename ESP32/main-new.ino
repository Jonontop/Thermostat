#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── PINS ──
#define DHTPIN       2
#define RELAY_PIN    5
#define SDA_PIN      8
#define SCL_PIN      9
#define ENC_L        1   
#define ENC_R        0   

Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(DHTPIN, DHT11);

// ── SHARED VARIABLES (volatile for interrupt safety) ──
volatile float targetTemp = 22.0;
volatile bool needsDisplayUpdate = true;
volatile unsigned long lastTurnTime = 0;

// ── MISSING GLOBALS ──
float currentTemp = 0.0;
bool  relayState  = false;
unsigned long lastDHTRead = 0;
unsigned long lastAPISync = 0;

// ── ENCODER INTERRUPT ──
void IRAM_ATTR readEncoder() {
  unsigned long now = millis();
  if (now - lastTurnTime > 50) { 
    // Logic flipped to invert rotation direction
    if (digitalRead(ENC_R) == digitalRead(ENC_L)) {
      if (targetTemp < 30.0) targetTemp += 0.5; // Now Increases
    } else {
      if (targetTemp > 5.0) targetTemp -= 0.5;  // Now Decreases
    }
    lastTurnTime = now;
    needsDisplayUpdate = true;
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  pinMode(ENC_L, INPUT_PULLUP);
  pinMode(ENC_R, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed");
  }
  display.clearDisplay();
  display.display();

  dht.begin();
  
  // Attach interrupt to ENC_L
  attachInterrupt(digitalPinToInterrupt(ENC_L), readEncoder, FALLING);
  
  WiFi.begin("SSID", "PASSWORD"); // <-- UPDATE THESE
}

void loop() {
  unsigned long now = millis();

  // 1. Handle Display (Keeps the main loop smooth)
  if (needsDisplayUpdate) {
    needsDisplayUpdate = false;
    Serial.print("Target changed to: "); Serial.println(targetTemp);
    updateDisplay();
  }

  // 2. Sensor Read & Relay Control (Every 3 seconds)
  if (now - lastDHTRead >= 3000) {
    lastDHTRead = now;
    float t = dht.readTemperature();
    if (!isnan(t)) {
      currentTemp = t;
      // Hysteresis logic
      if (currentTemp < (targetTemp - 0.3)) relayState = true;
      else if (currentTemp > (targetTemp + 0.3)) relayState = false;
      digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    }
    needsDisplayUpdate = true;
  }

  // 3. API Sync (Every 10 seconds)
  if (now - lastAPISync >= 10000) {
    lastAPISync = now;
    syncWithAPI();
  }
}

void syncWithAPI() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.setTimeout(1500); 
  http.begin("http://api.pecar.site/api/state");
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["temperature"] = currentTemp;
  doc["target"] = targetTemp;
  doc["relay_on"] = relayState;

  String body;
  serializeJson(doc, body);
  http.POST(body);
  http.end();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("THERMOSTAT");
  
  display.setCursor(0, 15);
  display.setTextSize(2);
  display.print("NOW: "); display.print(currentTemp, 1);
  
  display.setCursor(0, 40);
  display.print("SET: "); display.print(targetTemp, 1);
  
  if (relayState) {
    display.fillRect(90, 48, 38, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(93, 51);
    display.setTextSize(1);
    display.print("HEAT");
  }
  display.display();
}
