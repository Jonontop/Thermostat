# Thermostat Monitoring System — Documentation

A full-stack smart thermostat system combining an ESP32 firmware device, a Python/Flask REST API, and a browser-based dashboard.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Firmware — `main.ino`](#firmware--mainino)
3. [Backend — `app.py`](#backend--apppy)
4. [Frontend — `index.html`](#frontend--indexhtml)
5. [API Reference](#api-reference)
6. [Setup & Running](#setup--running)
7. [Security Notes](#security-notes)

---

## Architecture Overview

```
┌─────────────────────┐        WebSocket / HTTPS        ┌──────────────────┐
│   ESP32 Device      │ ──────────────────────────────▶ │  Sinric Pro Cloud│
│  DHT11 + Relay      │                                  └──────────────────┘
│  OLED Display       │
└─────────────────────┘
         │
         │  (future: direct HTTP or MQTT)
         ▼
┌─────────────────────┐          REST (HTTP/JSON)        ┌──────────────────┐
│  Flask API          │ ◀──────────────────────────────▶ │  Browser UI      │
│  SQLite DB          │                                  │  index.html      │
└─────────────────────┘                                  └──────────────────┘
```

The ESP32 device reads temperature from a DHT11 sensor and controls a relay based on the active mode and threshold. It communicates with the Sinric Pro cloud service for remote control. The Flask API independently manages state persistence and history logging for the web dashboard. The frontend polls the API every 30 seconds for current state and every 60 seconds for history.

---

## Firmware — `main.ino`

The Arduino sketch runs on an **ESP32** microcontroller and handles all hardware interaction.

### Hardware Requirements

| Component | Detail |
|---|---|
| Microcontroller | ESP32 |
| Temperature sensor | DHT11 (data pin: GPIO 15) |
| Relay module | GPIO 5 |
| Display | 128×64 SSD1306 OLED (I²C address `0x3C`) |

### Dependencies (Arduino Libraries)

- `Wire` — I²C communication for the OLED
- `WiFi` — ESP32 Wi-Fi stack
- `SinricPro` + `SinricProThermostat` — Cloud control integration
- `Adafruit_Sensor` + `DHT` — DHT11 sensor driver
- `Adafruit_GFX` + `Adafruit_SSD1306` — OLED display driver

### Configuration Constants

| Constant | Default | Description |
|---|---|---|
| `DHTPIN` | `15` | GPIO pin for DHT11 data line |
| `DHTTYPE` | `DHT11` | Sensor model |
| `RELAY_PIN` | `5` | GPIO pin that drives the relay |
| `thresholdTemperature` | `22.0` | Initial target temperature (°C) |
| `powerRate` | `0.1` | Power consumption rate (kWh per hour active) |

### Thermostat Modes

The firmware supports three modes, controllable via Sinric Pro:

| Mode | Relay Behavior |
|---|---|
| `heat` | Relay ON when `currentTemp < threshold` |
| `cool` | Relay ON when `currentTemp > threshold` |
| `off` | Relay always OFF regardless of temperature |

### Key Functions

**`updateRelayState()`**
Evaluates the current mode and temperature against the threshold, drives the relay HIGH or LOW accordingly, and accumulates power consumption. If the relay state changes, it fires a `sendPowerStateEvent` back to Sinric Pro.

**`updateTemperature()`**
Reads the DHT11 sensor every 5 seconds (enforced in `loop()`). On a valid reading it pushes the value to Sinric Pro via `sendTemperatureEvent`, calls `updateRelayState()`, and refreshes the OLED with current temperature, threshold, and mode.

**Sinric Pro Callbacks**

| Callback | Trigger | Action |
|---|---|---|
| `onPowerState` | Remote power toggle | Sets relay; respects mode |
| `onTargetTemperature` | Remote setpoint change | Updates `thresholdTemperature`, re-evaluates relay |
| `onThermostatMode` | Remote mode change | Updates `thermostatMode`, re-evaluates relay, echoes mode event |

### `setup()` Flow

1. Start serial at 115200 baud.
2. Connect to Wi-Fi (blocks until connected).
3. Initialise DHT sensor and relay pin (default LOW/off).
4. Initialise SSD1306 display; halt on failure.
5. Register Sinric Pro callbacks and call `SinricPro.begin()`.

### `loop()` Flow

1. Call `SinricPro.handle()` on every iteration to process cloud events.
2. Every 5 000 ms call `updateTemperature()`.

---

## Backend — `app.py`

A lightweight **Flask** REST API that persists thermostat state and history in a local SQLite database.

### Requirements

```
flask
flask-cors
```

### Database

SQLite file: `thermostat.db` (created automatically on first run).

**`thermostat_state`** — single-row current state

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | Always `1` |
| `is_on` | INTEGER | `1` = active, `0` = off |
| `temperature` | REAL | Target temperature (°C) |
| `updated_at` | TEXT | ISO 8601 UTC timestamp |

**`thermostat_log`** — append-only history

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | Auto-increment |
| `is_on` | INTEGER | State at log time |
| `temperature` | REAL | Temperature at log time |
| `recorded_at` | TEXT | ISO 8601 UTC timestamp |

### Seed Data

On first start, `init_db()` seeds:
- One row in `thermostat_state` with `is_on = 1`, `temperature = 21.0`.
- 168 hourly entries (7 days) in `thermostat_log` simulating a realistic on/off pattern (off between 23:00–06:00, with randomised temperatures per period).

### Temperature Clamping

All `POST /api/state` requests clamp the incoming temperature to **10.0 – 35.0 °C** server-side before persisting.

---

## Frontend — `index.html`

A self-contained single-page dashboard built with vanilla JavaScript, Bootstrap 5, Tailwind CSS (CDN Play build), and Chart.js.

### Features

- **Live status card** — Displays current temperature (large monospaced readout), on/off status pill with animated pulse dot, and last-updated timestamp.
- **Controls** — Temperature slider (10–35 °C) and power toggle switch; changes are staged locally and committed only when **Apply Changes** is clicked.
- **Stat chips** — Shows average, minimum, and maximum temperature computed from the loaded history window.
- **Activity chart** — Time-series line chart (Chart.js) with gradient fill showing temperature over the selected period.
- **Timeline chart** — Bar chart showing on/off activity per hour, colour-coded green/red.
- **Time range selector** — Buttons to filter charts to the last 24 h, 48 h, or 7 days.
- **Dark / light theme** — Toggle button in the top bar; CSS custom properties drive all colours.
- **Demo / offline mode** — If the API is unreachable, the frontend falls back to locally generated demo data and displays a warning toast.

### Polling Intervals

| Data | Interval |
|---|---|
| Current state (`GET /api/state`) | 30 seconds |
| History (`GET /api/history`) | 60 seconds |

### API Base URL

Configured at the top of the `<script>` block:

```js
const API = "http://localhost:5000/api";
```

Change this constant to point at a remote server when deploying.

### Chart.js Adapters

The time-axis charts require the `chartjs-adapter-date-fns` bundle, loaded from CDN alongside Chart.js itself.

---

## API Reference

All endpoints return and accept `application/json`.

### `GET /api/state`

Returns the current thermostat state.

**Response**

```json
{
  "is_on": true,
  "temperature": 21.5,
  "updated_at": "2025-04-01T10:30:00.000000"
}
```

### `POST /api/state`

Updates thermostat state. Both fields are optional; omitted fields keep their current value.

**Request body**

```json
{
  "is_on": false,
  "temperature": 19.0
}
```

**Response** — same shape as `GET /api/state`, reflecting the saved values.

**Notes**
- `temperature` is clamped to `[10.0, 35.0]`.
- Every call appends a row to `thermostat_log`.

### `GET /api/history`

Returns historical log entries in ascending chronological order.

**Query parameters**

| Parameter | Type | Default | Description |
|---|---|---|---|
| `limit` | integer | `168` | Maximum number of entries to return |

**Response**

```json
[
  {
    "is_on": true,
    "temperature": 20.3,
    "recorded_at": "2025-03-25T08:00:00.000000"
  }
]
```

---

## Setup & Running

### 1. Flash the Firmware

1. Open `main.ino` in the Arduino IDE (or PlatformIO).
2. Install the libraries listed under [Dependencies](#dependencies-arduino-libraries).
3. Update the Wi-Fi credentials and Sinric Pro keys in the sketch (see [Security Notes](#security-notes)).
4. Select your ESP32 board and upload.

### 2. Start the Backend

```bash
pip install flask flask-cors
python app.py
```

The API will be available at `http://localhost:5000`. The database and seed data are created automatically on first run.

### 3. Open the Dashboard

Open `index.html` directly in a browser. Make sure the `API` constant in the script block matches the address where `app.py` is running.

```js
// index.html — top of <script>
const API = "http://localhost:5000/api";
```

---

## Security Notes

> ⚠️ **The `main.ino` file currently contains plaintext credentials** (Wi-Fi SSID/password and Sinric Pro App Key/Secret). Before committing to version control or sharing the file, move these values to a separate header that is excluded via `.gitignore`, or use a secrets management approach such as storing them in ESP32 NVS (non-volatile storage).

Recommended pattern:

```cpp
// secrets.h  ← add to .gitignore
#define WIFI_SSID     "your_ssid"
#define WIFI_PASSWORD "your_password"
#define APP_KEY       "your_app_key"
#define APP_SECRET    "your_app_secret"
#define THERMOSTAT_ID "your_device_id"
```

```cpp
// main.ino
#include "secrets.h"
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;
// ...
```
