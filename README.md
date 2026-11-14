# 🌍 ENVIGUARD-X AI
### Intelligent IoT Environmental Monitoring System

An ESP32-powered smart environmental monitoring project designed to monitor temperature and humidity in real time, visualize sensor readings, and provide intelligent warning indications through a web dashboard and hardware components.

---

## 🚀 Project Overview

ENVIGUARD-X AI integrates IoT hardware and web technologies to provide real-time environmental monitoring.

The system collects temperature and humidity data using a DHT22 sensor connected to an ESP32. Sensor readings are displayed on an OLED screen and an interactive web dashboard.

When temperature reaches configured warning levels, the system activates corresponding LED indicators and a buzzer for critical temperature alerts.

## ✨ Key Features

- 🌡️ Real-time temperature monitoring
- 💧 Live humidity monitoring
- 📟 OLED display for sensor readings
- 🌐 ESP32-powered web dashboard
- 🚦 Three-level LED warning system
- 🔔 Critical temperature buzzer alert
- 📊 Temperature and humidity analytics
- 📈 Live waveform graphs
- 🧠 Rule-based environmental assessment
- 📁 CSV data export
- 📱 Responsive dashboard interface
- 📡 Local Wi-Fi communication
- ⚡ No paid cloud services required

## 🛠️ Hardware Requirements

| Component | Purpose |
|---|---|
| ESP32-WROOM-32 | Main controller |
| DHT22 | Temperature and humidity sensing |
| OLED 128×64 | Sensor data display |
| Green LED | Low temperature indication |
| Blue LED | Medium temperature indication |
| Red LED | High temperature indication |
| Buzzer | Critical temperature alert |
| Breadboard | Circuit connections |
| Jumper wires | Hardware connections |

## 💻 Software Requirements

- Arduino IDE
- ESP32 Board Package
- HTML, CSS and JavaScript
- C/C++ (Arduino programming)
- Wi-Fi connectivity

### Required Arduino Libraries

- WiFi.h
- WebServer.h
- Wire.h
- DHT.h
- Adafruit GFX Library
- Adafruit SSD1306

## 🌡️ Temperature Warning Levels

| Temperature | Level | Indicator |
|---|---|---|
| Below 30°C | LOW | 🟢 Green LED |
| 30°C–39.9°C | MEDIUM | 🔵 Blue LED |
| 40°C and above | HIGH | 🔴 Red LED + Buzzer |

*These are the configured thresholds in the dashboard design.*

## 🏗️ System Architecture

1. DHT22 collects environmental temperature and humidity.
2. ESP32 processes sensor readings.
3. OLED displays the current sensor values.
4. ESP32 hosts a local web server.
5. The dashboard retrieves readings through a JSON API.
6. Warning levels are displayed through visual indicators.
7. Critical temperature triggers the hardware alert system.
8. Sensor history can be exported as CSV.

## 📊 Dashboard Modules

- Live Environmental Overview
- Temperature and Humidity Monitoring
- Temperature Trend Analysis
- Three-Level Warning System
- Animated Temperature Gauge
- Temperature and Humidity Waveforms
- Minimum, Maximum and Average Analytics
- Device Health Monitoring
- Environmental Assessment
- Live Event Timeline
- CSV Data Export

## ⚙️ Installation and Setup

1. Install Arduino IDE.
2. Install the ESP32 board package.
3. Install the required Arduino libraries.
4. Connect the DHT22 sensor, OLED and alert components.
5. Configure your Wi-Fi credentials in the Arduino code.
6. Select the correct ESP32 board and COM port.
7. Upload the program.
8. Open Serial Monitor at 115200 baud.
9. Find the ESP32 IP address.
10. Open the IP address in a browser connected to the same Wi-Fi network.

## 🔮 Future Enhancements

- Historical environmental data storage
- Advanced predictive analytics
- Mobile application integration
- Remote monitoring capabilities
- Additional environmental sensors
- Cloud-based data visualization

## 🎯 Applications

- Smart homes
- Classrooms and laboratories
- Indoor environmental monitoring
- Server rooms
- IoT-based academic projects
- Environmental awareness systems

## 👨‍💻 Project Information

**Project Name:** ENVIGUARD-X AI  
**Domain:** Internet of Things (IoT)  
**Controller:** ESP32-WROOM-32  
**Sensor:** DHT22  
**Platform:** Arduino IDE  
**Dashboard:** HTML, CSS and JavaScript  
**Institution:** K. Ramakrishnan College of Engineering (KRCE)

---

### ⭐ Project Vision

To develop an affordable and accessible IoT-based environmental monitoring solution that combines real-time sensing, visual analytics and automated warning indications.

**Built with passion for IoT, innovation and smart technology.**
