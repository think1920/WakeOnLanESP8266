# ESP8266 Wake-on-LAN Controller with Integrated Weather Station and Information Display

## Abstract

This project implements a multifunctional IoT device based on the ESP8266 (NodeMCU or Wemos D1 Mini) microcontroller that serves primarily as a **Wake-on-LAN (WOL)** management interface while simultaneously operating as a compact desktop weather station and system information display. The device hosts a responsive web server allowing users to register, edit, and remotely power-on multiple PCs via Magic Packet, with persistent storage of device records in LittleFS. Additionally, it periodically retrieves current weather conditions for a predefined location via the OpenWeatherMap API and presents both meteorological and system data on a 20×4 I2C LCD in an alternating manner.

The implementation emphasizes code clarity, power efficiency, and user experience, and long-term reliability, making it suitable for both personal deployment and as a reference design for embedded web server applications on resource-constrained platforms.

## Features

- **Wake-on-LAN Management Web Interface**  
  - Full CRUD operations (Create, Read, Update, Delete) for PC entries (name, MAC address, optional IP)  
  - Persistent storage in `/pc_list.json` using LittleFS  
  - Responsive Bootstrap 4 UI with Font Awesome icons and real-time list updates via Fetch API  
  - Instant Magic Packet transmission using the WakeOnLan library  

- **Weather Station Functionality**  
  - Automatic retrieval of current temperature, humidity, atmospheric pressure, and wind speed from OpenWeatherMap  
  - Configurable location (default: Ho Chi Minh City, Vietnam) and update interval (10 s in current configuration)  

- **LCD Information Display (20×4 I2C)**  
  - Alternating screens (random interval 5–10 s):  
    1. Weather screen: Temperature (°C), Humidity (%), Pressure (hPa), Wind speed (m/s)  
    2. System screen: Current time/date (NTP synchronized, UTC+7), Local IP address, Free heap memory (bytes)  
  - Live clock updates every second when system screen is active  

- **Wi-Fi Configuration**  
  - WiFiManager portal on first boot or after explicit reset  
  - Reset Wi-Fi credentials button in web interface  

- **Power Optimization**  
  - Strategic `delay(1)` in main loop reducing idle power consumption by ≈60 % (ref. Hackaday 2022 study)  

## Hardware Requirements

- ESP8266 development board (NodeMCU, Wemos D1 Mini, etc.)
- 20×4 LCD with I2C backpack (address 0x27)
- Stable 5 V / ≥1 A power supply recommended for long-term operation

## Required Libraries (Arduino IDE / PlatformIO)

```text
ESP8266WiFi
ESP8266WebServer
LittleFS
WiFiUdp
WakeOnLan
ArduinoJson (>=6)
WiFiManager
ESP8266HTTPClient
NTPClient
LiquidCrystal_I2C
```

All libraries are available via the Arduino Library Manager.

## Configuration

1. **OpenWeatherMap API Key**  
   Replace `"xxxx"` in the source code with your personal API key:
   ```cpp
   String apiKey = "your_api_key_here";
   ```

2. **Location (optional)**  
   Modify if desired:
   ```cpp
   String city = "Ho%20Chi%20Minh";
   String countryCode = "VN";
   ```

3. **Timezone**  
   Currently set to UTC+7 (Indochina Time):
   ```cpp
   NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);
   ```

## Usage

1. Upload the sketch to the ESP8266.
2. On first boot, the device creates an access point named **WOL-ESP8266**. Connect to it and configure Wi-Fi credentials.
3. After connection, the LCD displays the assigned IP address.
4. Open `http://wol.local` or the displayed IP in a browser.
5. Add PCs via the "+" button, then wake them using the play button.

## File System

The device stores registered computers in `/pc_list.json` on LittleFS:
```json
[
  {
    "name": "Desktop-Gaming",
    "mac": "00:11:22:33:44:55",
    "ip": "192.168.1.100"
  }
]
```

## Web Interface Details

The interface is entirely self-contained (no external CDN dependency after initial load) and features:
- Instant list refresh after any modification
- Success/info/danger notifications with auto-dismiss
- Edit modal with delete confirmation
- Wi-Fi reset capability

## Academic / Technical Notes

- The project demonstrates effective use of modern JavaScript (Fetch, async JSON handling) within an embedded environment.
- Persistent data management on SPIFFS/LittleFS with proper error handling and schema validation.
- Graceful handling of network failures and JSON parsing errors.
- Balanced trade-off between display update frequency and network/CPU load.
- Significant power reduction through minimal delay in the main loop — particularly relevant for battery or solar-powered deployments.
