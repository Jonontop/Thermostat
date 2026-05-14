#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── PINS ──
#define DHTPIN       2
#define RELAY_PIN     5
#define SDA_PIN       8
#define SCL_PIN       9
#define ENC_L         1   
#define ENC_R         0   

// ── CONFIG ──
const char* ssid = "Zlatko";
const char* pass = "GalAnzelak2012";
const char* serverApi = "http://api.pecar.site/api/state";

Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(DHTPIN, DHT11);

// ── GLOBALS ──
volatile float targetTemp = 22.0;
volatile bool needsDisplayUpdate = true;
volatile bool localChangeMade = false;
volatile unsigned long lastTurnTime = 0;

float currentTemp = 0.0;
bool  relayState  = false;
unsigned long lastDHTRead = 0;
unsigned long lastAPISync = 0;

// ── ENCODER INTERRUPT ──
void IRAM_ATTR readEncoder() {
  unsigned long now = millis();
  unsigned long timeDiff = now - lastTurnTime;

  // 1. Debounce: ignore noise faster than 35ms
  if (timeDiff > 35) { 
    
    // 2. Acceleration: Determine step size based on speed
    float step = 0.5;
    if (timeDiff < 75) {
      step = 2.0; // Fast turn
    } else if (timeDiff < 150) {
      step = 1.0; // Moderate turn
    }

    // 3. Directional Logic (Inverted per your setup)
    if (digitalRead(ENC_R) == digitalRead(ENC_L)) {
      if (targetTemp + step <= 30.0) targetTemp += step;
      else targetTemp = 30.0;
    } else {
      if (targetTemp - step >= 5.0) targetTemp -= step;
      else targetTemp = 5.0;
    }

    // 4. Update State
    lastTurnTime = now;
    needsDisplayUpdate = true;
    localChangeMade = true; // Signals ESP32 to sync with Flask
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
  
  dht.begin();
  attachInterrupt(digitalPinToInterrupt(ENC_L), readEncoder, FALLING);
  
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) return;

  // 1. Refresh Display
  if (needsDisplayUpdate) {
    needsDisplayUpdate = false;
    updateDisplay();
  }

  // 2. Sensor & Hysteresis Logic (3s)
  if (now - lastDHTRead >= 3000) {
    lastDHTRead = now;
    float t = dht.readTemperature();
    if (!isnan(t)) {
      currentTemp = t;
      if (currentTemp < (targetTemp - 0.3)) relayState = true;
      else if (currentTemp > (targetTemp + 0.3)) relayState = false;
      digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
      needsDisplayUpdate = true;
    }
  }

  // 3. API Sync (10s)
  if (now - lastAPISync >= 10000) {
    lastAPISync = now;
    syncWithAPI();
  }
}

void syncWithAPI() {
  HTTPClient http;
  http.begin(serverApi);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["temperature"] = currentTemp;
  doc["target"] = targetTemp; // Server looks for 'target' from ESP
  doc["relay_on"] = relayState;

  String jsonStr;
  serializeJson(doc, jsonStr);

  int httpCode;
  if (localChangeMade) {
    httpCode = http.POST(jsonStr); // Push our new knob value
    if (httpCode == 200) localChangeMade = false; 
    Serial.println("POST: Sent knob change");
  } else {
    httpCode = http.GET(); // Just get updates from web
  }

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<200> response;
    deserializeJson(response, payload);
    
    // Update local target from DB (unless we are currently turning the knob)
    if (!localChangeMade && response.containsKey("target_temp")) {
        float webTarget = response["target_temp"];
        if (abs(webTarget - targetTemp) > 0.1) {
            targetTemp = webTarget;
            needsDisplayUpdate = true;
            Serial.println("Sync: Applied web change to local");
        }
    }
  }
  http.end();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  //display.setTextSize(1);
  //display.setCursor(0,0);
  //display.print("SMART THERMOSTAT");
  
  display.setCursor(0, 2);
  display.setTextSize(2);
  display.print("NOW:"); display.print(currentTemp, 1); display.print("C");
  
  display.setCursor(0, 42);
  display.print("SET:"); display.print(targetTemp, 1); display.print("C");
  
  if (relayState) {
    display.fillRect(95, 45, 30, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(98, 49);
    display.setTextSize(1);
    display.print("ON");
  }
  display.display();
}
