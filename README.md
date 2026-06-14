<div align="center">

# ⚡ IoT-Based Transformer Health Monitoring System

[![Arduino](https://img.shields.io/badge/Arduino-NodeMCU%20ESP8266-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![Python](https://img.shields.io/badge/Python-3.x-3776AB?style=for-the-badge&logo=python&logoColor=white)](https://python.org)
[![IoT](https://img.shields.io/badge/IoT-Enabled-3DDC84?style=for-the-badge&logo=internetofthings&logoColor=white)](https://github.com/mukkantiraviteja7)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)](LICENSE)

**A real-time transformer health monitoring system using IoT sensors, NodeMCU ESP8266, and a live web dashboard.**

[Features](#-features) · [Hardware](#-hardware-requirements) · [Circuit](#-circuit-diagram) · [Installation](#-installation) · [Dashboard](#-dashboard)

</div>

---

## 📌 Project Overview

Transformers are critical components in power distribution systems. Unexpected failures can cause massive power outages and financial losses. This **IoT-based Transformer Health Monitoring System** continuously monitors key parameters of a transformer in real-time and sends alerts when values exceed safe thresholds.

### Key Parameters Monitored
| Parameter | Sensor Used | Safe Range |
|---|---|---|
| 🌡️ Oil Temperature | DS18B20 | < 85°C |
| ⚡ Primary Voltage | ZMPT101B | 220V ± 10% |
| 🔌 Load Current | ACS712 (30A) | < 25A |
| 💧 Oil Level | HC-SR04 Ultrasonic | > 70% |
| 💨 Humidity | DHT11 | < 75% RH |
| ☢️ Gas/Fault Detection | MQ-2 | < 300 ppm |

---

## ✨ Features

- 📡 **Real-time Monitoring** — Live data from all sensors every 5 seconds
- 🚨 **Instant Alerts** — Buzzer + LED alarm when parameters exceed thresholds
- 📊 **Web Dashboard** — Live charts and gauges accessible from any device
- 📧 **Email Notifications** — Automatic email alerts for critical faults
- 📁 **Data Logging** — Sensor readings stored in CSV for analysis
- 🔋 **Low Power Design** — Deep sleep mode when all parameters are normal
- 📱 **Mobile Friendly** — Responsive dashboard works on phone and PC

---

## 🔧 Hardware Requirements

| Component | Quantity | Purpose |
|---|---|---|
| NodeMCU ESP8266 | 1 | Main microcontroller + WiFi |
| DS18B20 Temperature Sensor | 1 | Oil temperature measurement |
| ACS712 (30A) Current Sensor | 1 | Load current measurement |
| ZMPT101B Voltage Sensor | 1 | Primary voltage measurement |
| HC-SR04 Ultrasonic Sensor | 1 | Oil level monitoring |
| DHT11 Sensor | 1 | Humidity & ambient temperature |
| MQ-2 Gas Sensor | 1 | Fault gas / smoke detection |
| 16x2 LCD (I2C) | 1 | Local display |
| Buzzer | 1 | Audio alert |
| Red + Green LED | 2 | Status indicators |
| 4.7kΩ Resistor | 1 | DS18B20 pull-up |
| 5V Power Supply | 1 | System power |
| Breadboard + Jumper Wires | — | Connections |

---

## 🔌 Circuit Diagram

```
                    ┌─────────────────────────────────┐
                    │        NodeMCU ESP8266           │
                    │                                  │
 DS18B20 ──────────→│ D4  (GPIO2)   D1 (GPIO5) ──────→│ LCD SDA
 ACS712  ──────────→│ A0  (ADC)     D2 (GPIO4) ──────→│ LCD SCL
 ZMPT101B ─────────→│ A0  (ADC)     D5 (GPIO14)──────→│ Buzzer
 HC-SR04 TRIG ─────→│ D6  (GPIO12)  D7 (GPIO13)──────→│ Red LED
 HC-SR04 ECHO ─────→│ D7  (GPIO13)  D8 (GPIO15)──────→│ Green LED
 DHT11   ──────────→│ D3  (GPIO0)                     │
 MQ-2    ──────────→│ A0  (ADC)                       │
                    └─────────────────────────────────┘

 Power: VIN → 5V Supply | GND → Common Ground
```

### Pin Connections

| Sensor | NodeMCU Pin | Wire Color |
|---|---|---|
| DS18B20 DATA | D4 | Yellow |
| DS18B20 VCC | 3.3V | Red |
| DS18B20 GND | GND | Black |
| ACS712 OUT | A0 | Green |
| DHT11 DATA | D3 | Blue |
| HC-SR04 TRIG | D6 | Orange |
| HC-SR04 ECHO | D7 | White |
| MQ-2 AOUT | A0 | Purple |
| LCD SDA | D1 | Yellow |
| LCD SCL | D2 | Green |
| Buzzer + | D5 | Red |
| Red LED | D8 | Red |
| Green LED | D9 | Green |

---

## 💻 Installation

### Step 1 — Clone the Repository
```bash
git clone https://github.com/mukkantiraviteja7/IoT-Transformer-Health-Monitoring.git
cd IoT-Transformer-Health-Monitoring
```

### Step 2 — Arduino Setup
1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP8266 board: **File → Preferences → Board Manager URL:**
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Install required libraries via **Sketch → Include Library → Manage Libraries:**
   - `OneWire` by Paul Stoffregen
   - `DallasTemperature` by Miles Burton
   - `DHT sensor library` by Adafruit
   - `ESP8266WiFi` (included with ESP8266 board)
   - `LiquidCrystal_I2C` by Frank de Brabander
   - `ArduinoJson` by Benoit Blanchon

4. Open `arduino/transformer_monitor.ino`
5. Update WiFi credentials in the config section
6. Upload to NodeMCU

### Step 3 — Python Dashboard Setup
```bash
cd python
pip install -r requirements.txt
python dashboard.py
```
Open browser: `http://localhost:5000`

---

## 📊 Dashboard

The web dashboard displays:
- 📈 **Live Charts** — Temperature, voltage, current over time
- 🎯 **Gauge Meters** — Real-time values with color-coded zones
- 🚨 **Alert Log** — History of all fault events
- 📁 **Data Export** — Download readings as CSV

---

## 🚨 Alert Thresholds

| Parameter | Warning | Critical |
|---|---|---|
| Oil Temperature | > 75°C | > 85°C |
| Load Current | > 20A | > 25A |
| Oil Level | < 80% | < 70% |
| Gas Level | > 200 ppm | > 300 ppm |
| Humidity | > 65% RH | > 75% RH |

---

## 📁 Project Structure

```
IoT-Transformer-Health-Monitoring/
├── 📁 arduino/
│   └── transformer_monitor.ino   # NodeMCU firmware
├── 📁 python/
│   ├── dashboard.py              # Flask web dashboard
│   ├── data_collector.py         # Serial data reader
│   ├── alert_system.py           # Email alert system
│   └── requirements.txt          # Python dependencies
├── 📁 web/
│   └── dashboard.html            # Standalone web dashboard
├── 📁 docs/
│   └── circuit_diagram.md        # Detailed circuit info
├── 📄 README.md
└── 📄 LICENSE
```

---

## 🛠️ Technologies Used

- **Hardware:** NodeMCU ESP8266, DS18B20, ACS712, ZMPT101B, HC-SR04, DHT11, MQ-2
- **Firmware:** Arduino C++ (ESP8266 framework)
- **Backend:** Python 3, Flask
- **Frontend:** HTML5, CSS3, JavaScript, Chart.js
- **Communication:** WiFi (HTTP), Serial (USB)
- **Data Storage:** CSV logging

---

## 👨‍💻 Author

**Mukkanti Ravi Teja**
- 🎓 B.Tech EEE Student | Nellore, Andhra Pradesh
- 🐙 GitHub: [@mukkantiraviteja7](https://github.com/mukkantiraviteja7)
- 💼 LinkedIn: [mukkantiraviteja7](https://linkedin.com/in/mukkantiraviteja7)

---

## 📄 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

---

<div align="center">
⭐ If you found this project helpful, please give it a star!
</div>