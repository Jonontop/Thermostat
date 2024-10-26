#include <Wire.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



// DHT11 - 15
// RELAY - 5
// BUTTON1 - 18
// BUTTON2 - 19



// DHT11
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Rele
#define RELAY_PIN 5

// WiFi podatki
const char* ssid = "T-2_e8d3c9";
const char* password = "INNBOX3348309800939";

// Spletni strežnik
AsyncWebServer server(80);

float temperature = 0.0;
float thresholdTemperature = 22.0; // Temperaturni prag za vklop releja
bool relayState = false;

// GPIO for buttons
#define INCREASE_BUTTON_PIN 18
#define DECREASE_BUTTON_PIN 19
int buttonState_increase = 0;
int buttonState_decrease = 0;

// Display define
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create an instance of the SSD1306 display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// For debouncing
unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 300; // 300ms debounce delay

void setup() {
  Serial.begin(115200);

  // Povezava na WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Povezovanje na WiFi...");
  }
  Serial.println("WiFi povezan");

  // Inicializacija DHT11 in releja
  dht.begin();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Setup for buttons
  pinMode(INCREASE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DECREASE_BUTTON_PIN, INPUT_PULLUP); 


  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Nastavitev spletne strani
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><meta charset='UTF-8'></head><body><h1>Termostat</h1>";
    html += "<p class=\"temperature\">Trenutna temperatura: " + String(temperature) + " °C</p>";
    html += "<p class=\"threshold\">Trenutni prag za rele: " + String(thresholdTemperature) + " °C</p>";
    html += "<p class=\"relayStatus\">Rele je " + String(relayState ? "Vklopljen" : "Izklopljen") + "</p>";
    html += "<input type=\"range\" min=\"10\" max=\"40\" value=\"" + String(thresholdTemperature) + "\" id=\"thresholdSlider\" oninput=\"updateThreshold(this.value)\">";
    html += "<p>Prag releja: <span id=\"thresholdValue\">" + String(thresholdTemperature) + "</span> °C</p>";
    html += "<a href=\"/relay/on\"><button>Vklopi</button></a> ";
    html += "<a href=\"/relay/off\"><button>Izklopi</button></a>";
    html += "<script>";
    html += "function updateThreshold(value) {";
    html += "  document.getElementById('thresholdValue').innerHTML = value;";
    html += "  fetch('/setThreshold?value=' + value);";
    html += "}";
    html += "</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
});

  // Vklop releja
  server.on("/relay/on", HTTP_GET, [](AsyncWebServerRequest *request){
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    request->redirect("/");
  });

  // Izklop releja
  server.on("/relay/off", HTTP_GET, [](AsyncWebServerRequest *request){
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    request->redirect("/");
  });

  // Nastavitev temperaturnega praga
  server.on("/setThreshold", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      thresholdTemperature = value.toFloat();
    }
    request->send(200, "text/plain", "OK");
  });


  server.begin();
}

void loop() {
  // Branje temperature
  temperature = dht.readTemperature();
  if (isnan(temperature)) {
    //Serial.println("Napaka pri branju temperature!");
  } else {
    //Serial.println("Temperatura: " + String(temperature));
  }

  // Kontrola releja glede na prag
  if (temperature < thresholdTemperature && !relayState) {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
  } else if (temperature >= thresholdTemperature && relayState) {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
  }
  // Read the state of the button
  buttonState_increase = digitalRead(INCREASE_BUTTON_PIN);
  buttonState_decrease = digitalRead(DECREASE_BUTTON_PIN);


  // Check if the button is pressed
  if (buttonState_increase == LOW) {
    //Serial.println("Button1 is pressed");
    thresholdTemperature += 1.0;
  } 
  
  if (buttonState_decrease == LOW) {
    //Serial.println("Button2 is pressed");
    thresholdTemperature -= 1.0;
  } 

   // Update OLED display
  display.clearDisplay();
  
  // Display current temperature
  display.setCursor(0, 0);
  display.println("Current Temp:");
  display.setCursor(0, 10);
  display.print(temperature);
  display.println(" C");
  
  // Display threshold temperature
  display.setCursor(0, 30);
  display.println("Threshold Temp:");
  display.setCursor(0, 40);
  display.print(thresholdTemperature);
  display.println(" C");

  // Update the OLED display
  display.display();


  delay(500); // Osveževanje vsake 0.5 sekunde
}
