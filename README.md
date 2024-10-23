# Temperature Control System with ESP32 and remote fhisical remote

This project is a temperature control system utilizing the ESP32 microcontroller, DHT22 sensor, and a relay module. It enables users to monitor and control temperature through a web interface and API. The system can automatically adjust the relay state based on temperature readings, providing efficient climate control for various applications.

## Hypothesis

We hypothesize that implementing a web-based temperature control system will allow users to maintain desired environmental conditions more effectively. By using real-time data and user-defined thresholds, the system can optimize energy usage and enhance comfort.

## Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Code Overview](#code-overview)
- [API Endpoints](#api-endpoints)
- [Contributing](#contributing)
- [License](#license)

## Features

- Real-time temperature monitoring
- Adjustable temperature threshold
- Relay control based on temperature
- Web interface for user interaction
- RESTful API for programmatic access

## Hardware Requirements

- ESP32 microcontroller (only becouse of wifi, feel free to use any by your choise) 2x
- DHT22/11 temperature and humidity sensor
- Relay module (1 phase, more if needed)
- Breadboard and jumper wires
- Power supply for ESP32
- Display 2x
- Buttons 4x

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software)
- ESP32 board support in Arduino IDE
- Required libraries:
  - `WiFi`
  - `ESPAsyncWebServer`
  - `Adafruit_Sensor`
  - `DHT`

## Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/jonontop/Thermostat.git
   cd Thermostat
   ```
2. Install required libraries in Arduino IDE:

- Open Arduino IDE.
- Go to Sketch > Include Library > Manage Libraries.
- Search for and install DHT sensor library and ESPAsyncWebServer.

3. Configure WiFi credentials: Edit the code in main.ino to add your WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_SSID";
   const char* password = "YOUR_PASSWORD";
   ```

4. Upload the code:
- Connect your ESP32 to the computer via USB.
- Select the appropriate board and port in the Arduino IDE.
- Upload the code.

## Code Overview
The main components of the code include:
- DHT Sensor Initialization:
  ```cpp
  DHT dht(DHTPIN, DHTTYPE);
  ```
- Web Server Setup:
  ```cpp
  AsyncWebServer webServer(80);
  AsyncWebServer apiServer(81);
  ```

- Relay Control Logic: Uses digital pins to activate or deactivate the relay based on temperature readings.

## API Endpoints

- GET /fetch
  - Fetches data from an external source and returns a success message.
    
- GET /setThreshold
  - Updates the temperature threshold. Requires a value parameter.
 



# Slovenian

# Sistem za nadzor temperature z ESP32 in daljinskim upravljalnikom

Ta projekt je sistem za nadzor temperature, ki uporablja mikrokrmilnik ESP32, senzor DHT22 in relejni modul. Omogoča uporabnikom spremljanje in nadzor temperature prek spletnega vmesnika in API-ja. Sistem lahko samodejno prilagodi stanje releja na podlagi meritev temperature, kar zagotavlja učinkovito obvladovanje podnebja za različne aplikacije.

## Hipoteza

Hipotiziramo, da bo izvajanje sistema za nadzor temperature na osnovi spletnega vmesnika omogočilo uporabnikom, da bolj učinkovito vzdržujejo želeno okolje. Z uporabo podatkov v realnem času in uporabniško določenih pragov lahko sistem optimizira porabo energije in poveča udobje.

## Kazalo vsebine

- [Lastnosti](#lastnosti)
- [Strojne zahteve](#strojne-zahteve)
- [Programsko opremo zahteve](#programska-oprema-zahteve)
- [Namestitev](#namestitev)
- [Uporaba](#uporaba)
- [Pregled kode](#pregled-kode)
- [API končne točke](#api-kone-točke)
- [Pripravljenost](#pripravljenost)
- [Licenca](#licenca)

## Lastnosti

- Spremljanje temperature v realnem času
- Prilagodljiv temperaturni prag
- Nadzor releja na podlagi temperature
- Spletni vmesnik za interakcijo uporabnika
- RESTful API za programatični dostop

## Strojne zahteve

- Mikrokrmilnik ESP32 (samo zaradi WiFi, lahko uporabite katerikoli po svoji izbiri) 2x
- Senzor temperature in vlage DHT22/11
- Relejski modul (1 faza, več, če je potrebno)
- Breadboard in skakalne žice
- Napajalnik za ESP32
- Zaslon 2x
- Gumbi 4x

## Programsko opremo zahteve

- [Arduino IDE](https://www.arduino.cc/en/software)
- Podpora za ESP32 ploščo v Arduino IDE
- Zahtevane knjižnice:
  - `WiFi`
  - `ESPAsyncWebServer`
  - `Adafruit_Sensor`
  - `DHT`

## Namestitev

1. **Klonirajte repozitorij:**
   ```bash
   git clone https://github.com/jonontop/Thermostat.git
   cd Thermostat
   ```
2. Namestite zahtevane knjižnice v Arduino IDE:

- Odprite Arduino IDE.
- Pojdite na Sketch > Include Library > Manage Libraries.
- Iščite in namestite knjižnico DHT in ESPAsyncWebServer.

3. Konfigurirajte WiFi poverilnice: Uredite kodo v main.ino, da dodate svoje WiFi poverilnice:
   ```cpp
   const char* ssid = "YOUR_SSID";
   const char* password = "YOUR_PASSWORD";
   ```

4. Naložite kodo:
- Povežite svoj ESP32 z računalnikom prek USB.
- Izberite ustrezno ploščo in vrata v Arduino IDE.
- Naložite kodo.

## Pregled kode
Glavne komponente kode vključujejo:
- Inicializacija senzorja DHT:
  ```cpp
  DHT dht(DHTPIN, DHTTYPE);
  ```
- Nastavitev spletnega strežnika:
  ```cpp
  AsyncWebServer webServer(80);
  AsyncWebServer apiServer(81);
  ```

- Logika nadzora releja: Uporablja digitalne pine za aktiviranje ali deaktiviranje releja na podlagi meritev temperature.

## API končne točke

- GET /fetch
  - Pridobi podatke iz zunanjega vira in vrne sporočilo o uspehu.
    
- GET /setThreshold
  - Posodobi temperaturni prag. Zahteva parameter `value`.
 
