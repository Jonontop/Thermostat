# Pametni termostat z ESP32 in Sinric Pro
Projekt pametnega termostata z uporabo ESP32, temperaturnega senzorja DHT11, releja in OLED zaslona, integriranega z Google Home prek Sinric Pro. Sistem omogoča daljinsko upravljanje in nadzor temperature doma ter ogrevanja in hlajenja prek mobilne aplikacije ali glasovnega asistenta.

## Komponente
- ESP32: Glavni mikrokontroler, ki upravlja temperaturo in WiFi povezljivost.
- DHT11 senzor: Meri temperaturo okolice.
- Rele: Krmili napravo za ogrevanje ali hlajenje.
- OLED zaslon: Prikazuje trenutno temperaturo, ciljno temperaturo, način in porabo energije.
- Sinric Pro: Oblačna storitev za integracijo z Google Home ali Alexa, kar omogoča daljinsko in glasovno upravljanje.
## Funkcionalnosti
- Daljinsko upravljanje temperature: Nadzor ogrevanja/hlajenja prek Google Home ali aplikacije Sinric Pro.
### Načini termostata:
- heat (ogrevanje): Aktivira rele, če je trenutna temperatura pod ciljno.
- cool (hlajenje): Aktivira rele, če je trenutna temperatura nad ciljno.
- off (izklop): Rele je izklopljen.
- Sledenje porabi energije: Izračuna in prikazuje ocenjeno porabo energije med delovanjem releja.
- Posodobitve na OLED zaslonu: Prikazuje podatke v realnem času.
## Namestitev
### Namestitev knjižnic:
- WiFi, SinricPro, DHT, Adafruit_GFX, Adafruit_SSD1306.
- Konfiguracija Sinric Pro:
- Ustvarite račun na Sinric Pro, dodajte termostat in zabeležite appKey, appSecret in deviceID.
### Povezava komponent:
- DHT11: GPIO 15
- Rele: GPIO 5
- OLED: Priključite prek I2C (privzete I2C pine na ESP32)
## Pregled kode
### Povratni klici:
- onPowerState: Krmili stanje releja.
- onTargetTemperature: Nastavi ciljno temperaturo.
- onThermostatMode: Nastavi način termostata (heat, cool, off).
### Funkcije:
- updateTemperature(): Prebere temperaturo, posodobi prikaz in prilagodi stanje releja.
```cpp
void updateTemperature() {
    float temp = dht.readTemperature();
    if (!isnan(temp)) {
        currentTemperature = temp;
        myThermostat.sendTemperatureEvent(currentTemperature);

        updateRelayState(); // Update relay based on new temperature and mode

        ... 

        Serial.printf("Current Temperature: %.1f, Power Consumption: %.2f kWh\n", currentTemperature, powerConsumption);
    }
}
```
- updateRelayState(): Preveri in nastavi stanje releja glede na trenutni način in temperaturo ter izračuna porabo.
```cpp
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
```
## Uporaba
- Naložite kodo na ESP32 z ustreznimi WiFi in Sinric Pro podatki.
- Upravljajte temperaturo in spremljajte status prek Google Home ali aplikacije Sinric Pro.
- OLED zaslon bo prikazoval trenutno temperaturo, ciljno temperaturo, način in porabo energije.
## Odpravljanje težav
- Težave z WiFi: Preverite SSID in geslo.
```cpp
const char* ssid = "SSID";
const char* password = "Geslo";
```
- Povezava s Sinric Pro: Preverite pravilnost appKey, appSecret in deviceID.
```cpp
const char* appKey = "Ključ";
const char* appSecret = "Skrivnost";
const char* thermostatID = "ID/IME";
```
- Točnost temperature: Preverite povezave DHT11, če so odčitki nenatančni.
