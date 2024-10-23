# Temperature Control System with ESP32

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

- ESP32 microcontroller
- DHT22 temperature and humidity sensor
- Relay module
- Breadboard and jumper wires
- Power supply for ESP32

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
