#include <Wire.h>
#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProThermostat.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// DHT Sensor Settings
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Relay Pin for thermostat control
#define RELAY_PIN 5

// WiFi credentials
const char* ssid = "Zlatko";
const char* password = "GalAnzelak2012";

// Sinric Pro credentials
const char* appKey = "2b6cb211-ad2a-493c-8587-9fa91dc8752e";
const char* appSecret = "bc128207-7664-4af9-815b-12a88c8a6171-90930b6f-a4ab-4234-945a-5ef42cd7202c";
const char* thermostatID = "672169545889569a22b4fea6";

// Temperature control variables
float currentTemperature = 0.0;
float thresholdTemperature = 22.0;
bool relayState = false;
String thermostatMode = "off"; // Modes: "cool", "heat", "off"
float powerConsumption = 0.0;
float powerRate = 0.1;

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sinric Pro thermostat
SinricProThermostat& myThermostat = SinricPro[thermostatID];

// Sinric Pro power state callback
bool onPowerState(const String& deviceId, bool& state) {
    relayState = state && (thermostatMode != "off"); // Respect the mode setting
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    Serial.printf("Power State set to: %s\n", state ? "ON" : "OFF");
    return true;
}

// Sinric Pro target temperature callback
bool onTargetTemperature(const String& deviceId, float targetTemp) {
    Serial.printf("Setting threshold temperature to: %.1f\n", targetTemp);
    thresholdTemperature = targetTemp;
    updateRelayState();
    return true;
}

// Sinric Pro thermostat mode callback
bool onThermostatMode(const String& deviceId, String mode) {
    Serial.printf("Setting thermostat mode to: %s\n", mode.c_str());
    thermostatMode = mode;
    updateRelayState(); // Update relay based on the mode
    myThermostat.sendThermostatModeEvent(thermostatMode);
    return true;
}

// Update relay state based on mode and threshold
void updateRelayState() {
    bool previousState = relayState;

    if (thermostatMode == "heat" && currentTemperature < thresholdTemperature) {
        relayState = true;
    } else if (thermostatMode == "cool" && currentTemperature > thresholdTemperature) {
        relayState = true;
    } else {
        relayState = false;
    }

    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

    static unsigned long lastUpdate = millis();
    unsigned long currentTime = millis();
    if (relayState) {
        powerConsumption += powerRate * ((currentTime - lastUpdate) / 3600000.0);
    }
    lastUpdate = currentTime;

    if (relayState != previousState) {
        myThermostat.sendPowerStateEvent(relayState);
    }
}

// Read temperature, update display, and send data to Sinric Pro
void updateTemperature() {
    float temp = dht.readTemperature();
    if (!isnan(temp)) {
        currentTemperature = temp;
        myThermostat.sendTemperatureEvent(currentTemperature);

        updateRelayState(); // Update relay based on new temperature and mode

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Current Temp:");
        display.setCursor(0, 10);
        display.print(currentTemperature);
        display.println(" C");
        display.setCursor(0, 30);
        display.println("Threshold Temp:");
        display.setCursor(0, 40);
        display.print(thresholdTemperature);
        display.println(" C");
        display.setCursor(0, 50);
        display.print("Mode: ");
        display.print(thermostatMode);
        display.display();

        Serial.printf("Current Temperature: %.1f, Power Consumption: %.2f kWh\n", currentTemperature, powerConsumption);
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected");

    dht.begin();
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    myThermostat.onPowerState(onPowerState);
    myThermostat.onTargetTemperature(onTargetTemperature);
    myThermostat.onThermostatMode(onThermostatMode); // Set mode callback

    SinricPro.begin(appKey, appSecret);
}

void loop() {
    SinricPro.handle();

    static unsigned long lastTempUpdate = 0;
    if (millis() - lastTempUpdate > 5000) {
        lastTempUpdate = millis();
        updateTemperature();
    }
}
