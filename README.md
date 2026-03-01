# Wood Cutter - ESP32 Web-Controlled Machine
## 🪵 Overview

Wood Cutter is an embedded control system built around an ESP32 microcontroller.

The device acts simultaneously as:

- machine controller
- web server
- user interface

After connecting to WiFi, the machine exposes a web dashboard accessible from any phone or computer — no dedicated app required.

Open a browser → control the machine.


## ✨ Key Features
- Web-based control panel
- No mobile application required
- Local network operation
- Real-time command handling
- Modular embedded architecture
- Expandable for automation systems


## 💾 File System (LittleFS)
Frontend files are not embedded inside firmware. They are stored in ESP32 flash memory using LittleFS. This allows independent frontend development.

You can freely edit:
- HTML
- CSS
- JavaScript
- images
- icons
without touching firmware logic.

<br>

# 🚀 Getting Started
## 1. Configure WiFi

Create secrets.h in /web folder:

#define WIFI_SSID "YourWiFi"
#define WIFI_PASS "YourPassword"

## 2. You're ready to code
I love you Maria <3
