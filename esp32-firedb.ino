#include <Wire.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define RELAY_PIN 5

const char* ssid = "Zlatko";
const char* password = "GalAnzelak2012";

// Firebase configuration
#define FIREBASE_HOST "thermostat-7973a-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyA__qJ8hpZepkmqfKq4pjmGYLDTbUvOS9I" // Your Firebase API key

DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float thresholdTemperature = 22.0; 
bool relayState = false;

// Display define
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

AsyncWebServer server(80); // Create a web server on port 80

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected");

    // Initialize DHT11 and relay
    dht.begin();
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Setup the web server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = "<html><head><meta charset='UTF-8'></head><body><h1>Termostat</h1>";
        html += "<p class=\"temperature\">Current temperature: " + String(temperature) + " °C</p>";
        html += "<p class=\"threshold\">Current relay threshold: " + String(thresholdTemperature) + " °C</p>";
        html += "<p class=\"relayStatus\">Relay is " + String(relayState ? "On" : "Off") + "</p>";
        html += "<input type=\"range\" min=\"10\" max=\"40\" value=\"" + String(thresholdTemperature) + "\" id=\"thresholdSlider\" oninput=\"updateThreshold(this.value)\">";
        html += "<p>Relay threshold: <span id=\"thresholdValue\">" + String(thresholdTemperature) + "</span> °C</p>";
        html += "<a href=\"/relay/on\"><button>Turn On</button></a> ";
        html += "<a href=\"/relay/off\"><button>Turn Off</button></a>";
        html += "<script>";
        html += "function updateThreshold(value) {";
        html += "  document.getElementById('thresholdValue').innerHTML = value;";
        html += "  fetch('/setThreshold?value=' + value);";
        html += "}";

        // Add a script to send the temperature to Firebase
        html += "setInterval(function() {";
        html += "  fetch('/uploadTemperature');";
        html += "}, 5000);"; // Upload every 5 seconds

        html += "</script>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    // Relay on
    server.on("/relay/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        relayState = true;
        digitalWrite(RELAY_PIN, HIGH);
        request->redirect("/");
    });

    // Relay off
    server.on("/relay/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        relayState = false;
        digitalWrite(RELAY_PIN, LOW);
        request->redirect("/");
    });

    // Set temperature threshold
    server.on("/setThreshold", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            String value = request->getParam("value")->value();
            thresholdTemperature = value.toFloat();
        }
        request->send(200, "text/plain", "OK");
    });

    // Upload temperature to Firebase
    server.on("/uploadTemperature", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (WiFi.status() == WL_CONNECTED) {
            temperature = dht.readTemperature(); // Read temperature
            if (!isnan(temperature)) {
                // Create the URL for the Firebase database
                String url = "https://" FIREBASE_HOST "/temperature.json?auth=" FIREBASE_AUTH;

                // Create an HTTP client
                HTTPClient http;
                http.begin(url);
                http.addHeader("Content-Type", "application/json");

                // Prepare JSON data
                String jsonData = String("{\"temperature\":") + String(temperature) + "}";
                
                // Send a PUT request to update temperature
                int httpResponseCode = http.PUT(jsonData);

                if (httpResponseCode > 0) {
                    String response = http.getString(); // Get response payload
                    Serial.println("Response Code: " + String(httpResponseCode));
                    Serial.println("Response: " + response);
                } else {
                    Serial.println("Error on HTTP request: " + String(httpResponseCode));
                }

                // Close the connection
                http.end();
            }
        }
        request->send(200, "text/plain", "OK");
    });

    server.begin();
}

void loop() {
    // Read temperature
    temperature = dht.readTemperature();
    
    // Control relay based on threshold
    if (!isnan(temperature)) {
        if (temperature < thresholdTemperature && !relayState) {
            relayState = true;
            digitalWrite(RELAY_PIN, HIGH);
        } else if (temperature >= thresholdTemperature && relayState) {
            relayState = false;
            digitalWrite(RELAY_PIN, LOW);
        }

        // Update OLED display
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Current Temp:");
        display.setCursor(0, 10);
        display.print(temperature);
        display.println(" C");
        display.setCursor(0, 30);
        display.println("Threshold Temp:");
        display.setCursor(0, 40);
        display.print(thresholdTemperature);
        display.println(" C");
        display.display();
    }

    delay(5000); // Refresh every 5 seconds
}
