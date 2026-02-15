# ESP8266 Wake-on-LAN System with Environmental Monitoring

## Abstract

This project implements a network-based Wake-on-LAN (WOL) management system using the ESP8266 microcontroller. The system provides a web-based interface for remote device wake-up capabilities, integrated with real-time environmental monitoring through OpenWeatherMap API and an I2C LCD display. The implementation features secure authentication, persistent storage, and automatic WiFi configuration management.

## Table of Contents

1. Introduction
2. System Architecture
3. Hardware Requirements
4. Software Dependencies
5. Configuration
6. Installation and Setup
7. System Features
8. API Endpoints
9. Usage Instructions
10. Troubleshooting
11. Technical Specifications
12. License

## 1. Introduction

Wake-on-LAN (WOL) is a network protocol that enables remote power management of computers through magic packets. This implementation extends basic WOL functionality by providing:

- Web-based management interface with Bootstrap 4 UI
- Persistent device storage using LittleFS filesystem
- Environmental data visualization on 20x4 I2C LCD
- Automatic WiFi provisioning via WiFiManager
- HTTP Basic Authentication for security
- NTP time synchronization
- Weather API integration

## 2. System Architecture

### 2.1 Core Components

The system comprises four primary modules:

**Network Layer**: Handles WiFi connectivity, UDP packet transmission, HTTP server, and NTP synchronization.

**Storage Layer**: Manages persistent device configurations using LittleFS with JSON serialization.

**Presentation Layer**: Dual output system consisting of web interface (HTML/CSS/JavaScript) and LCD display.

**Data Acquisition Layer**: Interfaces with OpenWeatherMap API for environmental data retrieval.

### 2.2 System Workflow

```
User Request → Authentication → Web Server → JSON Processing → 
→ Device Storage/WOL Transmission → Response
```

### 2.3 Display System

The LCD operates in a dual-screen mode with automatic switching:

**Screen 0 (Weather)**: Temperature, humidity, atmospheric pressure, wind speed

**Screen 1 (System)**: Current time, date, IP address, free heap memory

Screen transitions occur at randomized intervals (5-15 seconds) to prevent burn-in and provide varied information display.

## 3. Hardware Requirements

### 3.1 Essential Components

- ESP8266 Development Board (NodeMCU, Wemos D1 Mini, or equivalent)
- 20x4 I2C LCD Display Module (Address: 0x27)
- Micro USB Cable for programming and power
- Stable 5V power supply (minimum 500mA recommended)

### 3.2 Connection Diagram

```
ESP8266          LCD Display
---------        -----------
GPIO 4 (D2) ---- SDA
GPIO 5 (D1) ---- SCL
GND ------------ GND
3.3V ----------- VCC
```

Note: Some I2C LCD modules require 5V VCC. Verify your module specifications.

## 4. Software Dependencies

### 4.1 Arduino IDE Libraries

Install the following libraries via Arduino Library Manager:

- ESP8266WiFi (Core library)
- ESP8266WebServer (Core library)
- ESP8266HTTPClient (Core library)
- LittleFS (Core library)
- WakeOnLan by a7md0 (v1.1.6 or higher)
- ArduinoJson by Benoit Blanchon (v6.x)
- WiFiManager by tzapu (v2.0.16-rc.2 or higher)
- NTPClient by Fabrice Weinberg (v3.2.0 or higher)
- LiquidCrystal_I2C by Frank de Brabander (v1.1.2 or higher)

### 4.2 Board Configuration

In Arduino IDE:
- Board: "NodeMCU 1.0 (ESP-12E Module)" or appropriate variant
- Flash Size: "4MB (FS:2MB OTA:~1019KB)"
- CPU Frequency: "80 MHz"
- Upload Speed: "115200"

## 5. Configuration

### 5.1 User Configuration Section

Modify the following parameters in the code header:

```cpp
// Authentication Credentials
const char* www_username = "admin";
const char* www_password = "your_password_here";

// Weather API Configuration
String apiKey = "your_openweathermap_api_key";
String city = "Ho%20Chi%20Minh";  // URL encoded
String countryCode = "VN";

// Network Configuration
String Hostname = "wol-system";
```

### 5.2 Obtaining OpenWeatherMap API Key

1. Register at https://openweathermap.org/api
2. Subscribe to "Current Weather Data" (free tier available)
3. Copy your API key from the dashboard
4. Replace the `apiKey` variable value

### 5.3 LCD I2C Address Configuration

Default address is 0x27. If your display uses a different address:

```cpp
LiquidCrystal_I2C lcd(0x3F, 20, 4);  // Alternative address
```

Use an I2C scanner sketch to detect your module's address if unsure.

## 6. Installation and Setup

### 6.1 Initial Upload

1. Connect ESP8266 to computer via USB
2. Open the .ino file in Arduino IDE
3. Configure user parameters (Section 5.1)
4. Select correct board and port
5. Upload the sketch

### 6.2 WiFi Provisioning

First boot sequence:

1. LCD displays "System Booting..."
2. ESP8266 creates WiFi Access Point: "WOL-ESP8266"
3. Connect to this AP using mobile device or computer
4. Captive portal opens automatically (or navigate to 192.168.4.1)
5. Select your WiFi network and enter credentials
6. ESP8266 reboots and connects to configured network

### 6.3 Accessing the Web Interface

1. Check LCD Screen 1 for assigned IP address
2. Open web browser and navigate to: http://[ESP8266_IP_ADDRESS]
3. Enter authentication credentials (default: admin/Thinh1906)
4. Web control panel will load

Alternative: Use hostname: http://wol-system.local (mDNS support required)

## 7. System Features

### 7.1 Device Management

**Add New Device**: Register computers with name, MAC address, and optional IP

**Edit Device**: Modify stored device information

**Delete Device**: Remove devices from persistent storage

**Wake Device**: Send WOL magic packet (transmitted 3 times for reliability)

### 7.2 Environmental Monitoring

Real-time weather data display:
- Temperature (Celsius)
- Relative humidity (percentage)
- Atmospheric pressure (hPa)
- Wind speed (m/s)

Update interval: 20 minutes (to comply with API rate limits)

### 7.3 System Information Display

- NTP-synchronized time (UTC+7 timezone)
- Current date (DD/MM/YYYY format)
- Local IP address
- Available heap memory

### 7.4 Security Features

- HTTP Basic Authentication on all endpoints
- Session-based credential validation
- Password-protected WiFi reset function
- No default credentials in public repositories

## 8. API Endpoints

### 8.1 Web Interface

```
GET /
Description: Main control panel interface
Authentication: Required
Response: HTML page
```

### 8.2 Device Operations

```
GET /pc_list
Description: Retrieve all stored devices
Authentication: Required
Response: JSON array of device objects

POST /add
Description: Add new device to system
Authentication: Required
Payload: {"name": "string", "mac": "string", "ip": "string"}
Response: 200 OK or error status

POST /edit
Description: Modify existing device
Authentication: Required
Payload: {"index": int, "name": "string", "mac": "string", "ip": "string"}
Response: 200 OK or 404 Not Found

POST /delete?index=[int]
Description: Remove device from storage
Authentication: Required
Response: 200 OK or 404 Not Found

POST /wake
Description: Send WOL magic packet
Authentication: Required
Payload: {"mac": "string"}
Response: 200 OK or 500 Internal Server Error
```

### 8.3 System Management

```
POST /reset_wifi
Description: Clear WiFi credentials and reboot
Authentication: Required
Response: 200 OK (device restarts)
```

## 9. Usage Instructions

### 9.1 Adding a Target Computer

Preparation on target computer:

1. Enable Wake-on-LAN in BIOS/UEFI settings
2. Enable WOL in network adapter properties (Windows) or ethtool (Linux)
3. Note the network adapter's MAC address

In web interface:

1. Click "Add New" button
2. Enter device name (e.g., "Gaming PC")
3. Enter MAC address in format: AA:BB:CC:DD:EE:FF
4. (Optional) Enter static IP address
5. Click "Add Device"

### 9.2 Waking a Computer

1. Locate device in the device list
2. Click "Wake" button
3. System sends three consecutive magic packets
4. Target computer should power on within 5-10 seconds

Note: Both devices must be on the same subnet for standard WOL operation.

### 9.3 Resetting WiFi Configuration

If network change is required:

1. Access web interface
2. Click "Reset" button (red WiFi icon)
3. Confirm reset action
4. ESP8266 clears credentials and reboots
5. Follow WiFi provisioning procedure (Section 6.2)

## 10. Troubleshooting

### 10.1 Common Issues

**Problem**: LCD shows no characters or random symbols

**Solution**: 
- Verify I2C address (use scanner sketch)
- Check SDA/SCL connections
- Ensure adequate power supply (LCD backlight draws significant current)

**Problem**: Cannot access web interface

**Solution**:
- Verify ESP8266 connected to WiFi (check LCD Screen 1)
- Confirm IP address is correct
- Try hostname: http://wol-system.local
- Check firewall settings on accessing device

**Problem**: WOL packets not waking target

**Solution**:
- Verify WOL enabled in target BIOS and OS
- Confirm MAC address is correct (no typos)
- Ensure both devices on same network segment
- Some routers block broadcast packets - check router settings
- Try waking via directed IP instead of broadcast

**Problem**: Weather data shows zeros

**Solution**:
- Verify API key is valid and active
- Check internet connectivity
- Confirm city name is URL-encoded correctly
- Check OpenWeatherMap API status

**Problem**: ESP8266 keeps rebooting

**Solution**:
- Power supply may be insufficient (use quality USB adapter)
- Check for short circuits in wiring
- Monitor serial output at 115200 baud for error messages

### 10.2 Serial Debugging

Connect to serial monitor at 115200 baud to view system logs including:
- WiFi connection status
- HTTP requests
- WOL packet transmission confirmations
- JSON parsing errors

## 11. Technical Specifications

### 11.1 Performance Characteristics

- HTTP Response Time: Less than 50ms for local operations
- WOL Packet Transmission: 150ms (three consecutive packets)
- Weather API Call: 1-2 seconds (blocking operation)
- LCD Update Rate: 1 Hz (system info screen), 20 minutes (weather screen)
- Maximum Stored Devices: Limited by available heap (approximately 50 devices)

### 11.2 Memory Usage

- Flash Usage: Approximately 380 KB program memory
- SRAM Usage: Approximately 35 KB at runtime
- LittleFS: 2 MB reserved for filesystem
- JSON Document Size: 2 KB (static), 512 B (dynamic operations)

### 11.3 Network Specifications

- WiFi Standards: 802.11 b/g/n (2.4 GHz only)
- WOL Protocol: UDP port 9
- HTTP Server Port: 80
- NTP Port: 123 (UDP)
- Maximum Concurrent Connections: 4

### 11.4 Environmental Operating Conditions

- Operating Temperature: 0°C to 70°C (ESP8266 specification)
- Recommended Environment: Indoor, temperature-controlled
- Power Consumption: 70-170mA during operation, 20mA idle

## 12. License

This project is provided as-is for educational and personal use. 

### Third-Party Licenses

This project incorporates open-source libraries with respective licenses:
- ESP8266 Arduino Core: LGPL 2.1
- ArduinoJson: MIT License
- WiFiManager: MIT License
- WakeOnLan Library: MIT License
- NTPClient: MIT License
- LiquidCrystal_I2C: Creative Commons

## Author

Developed by TrienChill (2026)

## Acknowledgments

This implementation builds upon the work of the ESP8266 community and utilizes several well-maintained open-source libraries. Weather data provided by OpenWeatherMap.

## References

1. Wake-on-LAN Protocol Specification: AMD Magic Packet Technology White Paper
2. ESP8266 Technical Reference: Espressif Systems
3. OpenWeatherMap API Documentation: https://openweathermap.org/api
4. I2C Protocol Specification: NXP Semiconductors
5. ArduinoJson Documentation: https://arduinojson.org/
