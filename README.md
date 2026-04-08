# YoloUNO PlatformIoT

An ESP32-S3 based IoT monitoring and control system built with FreeRTOS, featuring real-time sensor monitoring, web-based configuration, TinyML anomaly detection, and cloud telemetry publishing to CoreIOT.

## Overview

This project implements a complete IoT node on an ESP32-S3 board. Starting from basic LED control, the system is extended into a multi-task embedded application with environmental sensing using DHT20, status indication through LED, NeoPixel, and 16x2 I2C LCD, an Access Point web dashboard for device control and configuration, on-device TinyML inference, and cloud connectivity through MQTT to CoreIOT.

The system is designed around FreeRTOS tasks, with mutexes and semaphores used to safely share sensor data and device states across concurrent tasks.

## Main Features

### 1. Multi-task IoT architecture with FreeRTOS

The firmware is structured into independent tasks running concurrently, including:

- sensor monitoring
- LED control
- NeoPixel control
- LCD display
- TinyML inference
- web server
- Wi-Fi client
- CoreIOT cloud publishing

This makes the system modular, responsive, and scalable.

### 2. Temperature-based LED behavior

A shared temperature state is generated from DHT20 readings:

- COLD: `T < 25°C`
- NORMAL: `25°C <= T <= 32°C`
- HOT: `T > 32°C`

The onboard LED changes its blinking pattern depending on the current temperature state.

### 3. Humidity-based NeoPixel indication

Humidity is classified into three ranges:

- DRY: `H < 55%`
- COMFORT: `55% <= H <= 80%`
- WET: `H > 80%`

The NeoPixel color is updated accordingly:

- Yellow for DRY
- Green for COMFORT
- Blue for WET

### 4. LCD environment status display

A 16x2 I2C LCD shows temperature, humidity, and environment status:

- NORMAL
- WARNING
- CRITICAL

The LCD logic is based on combined temperature and humidity thresholds, with proper I2C bus protection using a mutex.

### 5. AP mode web dashboard

When Wi-Fi credentials are not available, the ESP32-S3 starts in Access Point mode and hosts a responsive web interface.

The web UI allows users to:

- monitor temperature and humidity in real time
- control Blink LED and NeoPixel
- configure Wi-Fi credentials
- configure CoreIOT server, port, and token
- save configuration into flash storage and reboot into STA mode

The dashboard uses:

- REST API (`/sensors`) for live data
- WebSocket (`/ws`) for real-time device control and settings update
- LittleFS for serving web files and storing config (`/info.dat`)

### 6. TinyML anomaly detection on device

A lightweight TinyML model is trained using real sensor data collected from the ESP32-S3.

- input features: temperature and humidity
- output: normal or anomaly
- deployed on device using TensorFlow Lite Micro

The model runs locally in a dedicated FreeRTOS task and performs inference directly on the ESP32-S3.

### 7. CoreIOT cloud integration

In STA mode, the board connects to Wi-Fi and publishes telemetry to CoreIOT via MQTT.

Published telemetry:

```json
{
  "temperature": 29.52,
  "humidity": 68.73
}
```

The device also supports RPC-based remote control from CoreIOT to enable or disable:

- Blink LED
- NeoPixel

## System Workflow

1. On startup, the device checks whether Wi-Fi/CoreIOT configuration exists in `/info.dat`.
2. If configuration is missing, the ESP32-S3 starts in AP mode and launches the web server.
3. The user connects to the AP, opens the dashboard, and enters Wi-Fi and CoreIOT settings.
4. The configuration is saved into LittleFS.
5. The board restarts and switches to STA mode.
6. The system connects to Wi-Fi, runs all local tasks, and starts publishing sensor telemetry to CoreIOT.

## Project Tasks

### Task 1 — Single LED Blink with Temperature Conditions

- Read temperature from DHT20
- Classify temperature into COLD, NORMAL, and HOT
- Update LED blink pattern safely using shared state and mutex protection

### Task 2 — NeoPixel LED Control Based on Humidity

- Read humidity from DHT20
- Classify humidity into DRY, COMFORT, and WET
- Update NeoPixel color based on shared humidity state

### Task 3 — Temperature and Humidity Monitoring with LCD Display

- Display real-time temperature and humidity on LCD
- Show NORMAL, WARNING, and CRITICAL status
- Protect shared state and I2C access with mutexes

### Task 4 — Wi-Fi Access Point Web Server and Device Configuration

- Start AP mode when credentials are missing
- Serve dashboard UI from LittleFS
- Control devices via WebSocket
- Read sensor data via REST API
- Save Wi-Fi and CoreIOT settings persistently

### Task 5 — TinyML Deployment and Accuracy Evaluation

- Collect real sensor dataset from hardware
- Label samples as normal or anomaly
- Train a lightweight neural network in Python
- Convert model to TensorFlow Lite Micro
- Run inference directly on ESP32-S3

### Task 6 — Data Publishing to CoreIOT Cloud Server

- Connect to Wi-Fi in STA mode
- Authenticate using CoreIOT access token
- Publish telemetry periodically via MQTT
- Support remote RPC control from dashboard widgets

## Hardware Used

- ESP32-S3
- DHT20 temperature and humidity sensor
- NeoPixel RGB LED
- 16x2 I2C LCD
- onboard status LED

## Software and Technologies

- C/C++
- FreeRTOS
- Arduino framework for ESP32
- LittleFS
- ESPAsyncWebServer
- WebSocket
- REST API
- TensorFlow Lite Micro
- MQTT
- CoreIOT Cloud

## Key Technical Points

- Concurrent task design on FreeRTOS
- Safe shared-state handling using mutexes and semaphores
- Producer-consumer pattern for sensor data flow
- I2C bus protection between DHT20 and LCD
- Real-time web UI served directly from the microcontroller
- Persistent configuration storage in flash
- On-device TinyML inference
- Cloud telemetry and RPC integration via MQTT

## Team Contributions

- **Phạm Lê Minh Khôi** — Overall system integration, implementation of Tasks 1–3
- **Huỳnh Vỹ Quốc** — Web UI design and Task 4 (AP web server)
- **Đỗ Tài Thành** — TinyML dataset preparation, Task 5 model training, and Task 6 cloud integration

## Result

This project evolved from a simple LED demo into a complete embedded IoT platform with:

- sensor monitoring
- local visual feedback
- responsive web configuration
- TinyML-based local intelligence
- cloud-based telemetry and remote control

All major functions were implemented and tested on real ESP32-S3 hardware.

## Future Improvements

- add more sensors and actuators
- improve dashboard visualization and historical storage
- optimize TinyML model size and inference speed
- extend cloud-side automation and alerting
- add OTA update workflow for easier deployment

## Repository Note

This repository contains the source code and related assets for the final IoT assignment project implemented on ESP32-S3.
