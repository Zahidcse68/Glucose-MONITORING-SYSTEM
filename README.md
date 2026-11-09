# Glucose-MONITORING-SYSTEM
An IoT-based Smart IV/Glucose Bottle Monitoring System using an ESP8266 and HX711 Load Cell. Features a sleek real-time web dashboard with visual liquid tracking, custom bottle calibration, EEPROM memory, and a low-level buzzer alarm.

# Smart IV / Glucose Bottle Monitor 💧🏥

An IoT-based medical fluid monitoring system built with the ESP8266 and an HX711 load cell. This project accurately measures the weight of an IV/Glucose bottle in real-time, calculates the remaining liquid volume, and triggers a local buzzer alarm when the fluid drops below a critical threshold. 

The system hosts a sleek, responsive, and mobile-friendly web dashboard directly from the ESP8266, featuring a dynamic visual representation of the bottle filling and emptying, alongside a comprehensive hidden calibration menu.

## ✨ Features

- **Real-Time Web Dashboard:** Monitor the bottle status dynamically on any browser (phone or PC) without needing a dedicated app.
- **Dynamic Visual UI:** The web interface features a bottle graphic where the liquid level physically rises, falls, and changes color (Blue for normal, Red for low/empty) based on real-time data.
- **Custom Bottle Calibration:** Easily calibrate the system for *any* bottle size by setting the empty bottle weight and the full bottle weight via the web interface.
- **EEPROM Memory:** Calibration data (scale factor, offset, bottle capacity) is saved to the ESP8266's EEPROM. It remembers its settings even after a power outage.
- **Low-Level Audible Alarm:** A local buzzer automatically triggers when the liquid drops below 40 grams (configurable).
- **Web-Based Tare & Scale Calibration:** Zero the scale and calibrate the HX711 using known standard weights entirely through the hidden settings menu.

## 🛠️ Hardware Requirements

- **ESP8266 Microcontroller** (NodeMCU or Wemos D1 Mini)
- **HX711** Load Cell Amplifier Module
- **Load Cell** (e.g., 1kg, 5kg)
- **Active Buzzer**
- 5V Power Supply
- Connecting wires & breadboard/PCB

## 🔌 Circuit Wiring / Pinout

| Component | ESP8266 (NodeMCU) Pin |
| :--- | :--- |
| **HX711 DT** (Data) | `D6` (GPIO 12) |
| **HX711 SCK** (Clock) | `D5` (GPIO 14) |
| **HX711 VCC** | `3V3` or `5V` (Depending on module) |
| **HX711 GND** | `GND` |
| **Buzzer (+)** | `D7` (GPIO 13) |
| **Buzzer (-)** | `GND` |

## 🚀 Installation & Setup

1. **Install Required Libraries:**
   Open the Arduino IDE, go to *Sketch -> Include Library -> Manage Libraries*, and install:
   - `HX711` by Bogdan Necula

2. **Configure Wi-Fi:**
   Open the sketch and update the Wi-Fi credentials to match your local network or mobile hotspot:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
