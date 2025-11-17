# ESP32 Sensor Dashboard

A comprehensive dashboard for monitoring ESP32 onboard sensors and system status on an OLED display.

## Features

The dashboard automatically cycles through 5 different display modes:

1. **Overview** - Shows key metrics at a glance:
   - CPU frequency
   - Free heap memory
   - Internal temperature
   - WiFi networks detected
   - System uptime

2. **WiFi Details** - Scans and displays nearby WiFi networks:
   - Number of networks found
   - Top 3 strongest networks with signal strength

3. **Memory Details** - Detailed memory information:
   - Total heap size
   - Free memory
   - Used memory
   - Visual memory usage bar graph

4. **Temperature** - ESP32 internal temperature sensor:
   - Large temperature display
   - Status indicator (Normal/Warm/Hot)

5. **System Info** - Hardware information:
   - Chip model
   - Number of cores
   - CPU frequency
   - Flash memory size
   - SDK version

## Hardware Requirements

- **ESP32 Development Board** (Heltec WiFi Kit 32 or similar)
- **OLED Display** (SSD1306, 128x64 pixels)
  - For Heltec WiFi Kit 32, the OLED is built-in

## Pin Configuration (Heltec WiFi Kit 32)

- SDA: GPIO 4
- SCL: GPIO 15
- RST: GPIO 16
- I2C Address: 0x3C

## Installation

1. Open this project in PlatformIO
2. Connect your ESP32 board
3. Build and upload:
   ```bash
   pio run --target upload
   ```
4. Open serial monitor to see debug output:
   ```bash
   pio device monitor
   ```

## Display Modes

The dashboard automatically cycles through each mode every 5 seconds. You can monitor the current mode through the serial output.

## Monitored Sensors

### Built-in Sensors
- **Internal Temperature Sensor** - Monitors ESP32 chip temperature
- **WiFi Radio** - Scans for nearby networks
- **Memory Monitor** - Tracks heap memory usage
- **CPU Monitor** - Reports frequency and core count

### System Metrics
- CPU frequency (MHz)
- Heap memory (total/free/used)
- Uptime (hours/minutes/seconds)
- WiFi signal scanning
- Flash memory size
- SDK version

## Customization

You can modify the display duration by changing the `MODE_DURATION` constant in [main.cpp](src/main.cpp):

```cpp
const unsigned long MODE_DURATION = 5000; // milliseconds
```

## Serial Output

The dashboard outputs debug information to the serial monitor at 115200 baud, including:
- Initialization status
- Mode changes
- Sensor readings

## Dependencies

- Adafruit SSD1306 (^2.5.7)
- Adafruit GFX Library (^1.11.3)
- Adafruit BMP280 Library (^2.6.8) - Optional for external sensors
- Adafruit Unified Sensor (^1.1.14)

## Future Enhancements

Potential additions:
- Button controls to manually switch modes
- External sensor support (BMP280, DHT22, etc.)
- Data logging to SD card
- Web interface for remote monitoring
- Configurable display settings
- Alert thresholds for temperature/memory

## License

MIT License
