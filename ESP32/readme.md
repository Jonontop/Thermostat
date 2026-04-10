/*
 * Thermostat Firmware — ESP32
 * Display : LCD 16×2 via I2C (PCF8574 backpack, default addr 0x27)
 * Sensor  : DHT11 on GPIO 15
 * Relay   : GPIO 5
 * Backend : Flask REST API (HTTP)
 *
 * Libraries required (install via Library Manager):
 *   - LiquidCrystal_I2C  (Frank de Brabander, v1.1.2+)
 *   - DHT sensor library  (Adafruit)
 *   - Adafruit Unified Sensor
 *   - ArduinoJson (Benoit Blanchon, v7.x)
 */
