# WiFi Signal Strength Meter for ESP32

A real-time WiFi signal strength visualizer for ESP32 with OLED display. Perfect for mapping WiFi coverage around your house!

## Features

- **Signal Meter Mode**: Live RSSI display with quality percentage and bar graph
- **Signal Graph Mode**: Real-time graph showing signal strength history
- **Network Scanner Mode**: Scan and display nearby WiFi networks
- Auto-cycles through all modes every 10 seconds
- Serial output for logging

## Hardware Requirements

- ESP32 development board
- 128x64 OLED display (SSD1306) with I2C connection
- USB cable for programming and power

## Default I2C Pins

The ESP32 uses these default I2C pins:
- **SDA**: GPIO 21
- **SCL**: GPIO 22
- **VCC**: 3.3V
- **GND**: GND

If your display uses different pins, you can change them in the code by adding:
```cpp
Wire.begin(SDA_PIN, SCL_PIN);  // Add before display.begin()
```

## Setup Instructions

### 1. Configure WiFi Credentials

Edit [src/main.cpp](src/main.cpp) and change these lines:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";        // Your WiFi network name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"; // Your WiFi password
```

### 2. Build and Upload

Using PlatformIO CLI:
```bash
cd wifi-signal-meter
pio run --target upload
pio device monitor
```

Or use the PlatformIO extension in VSCode:
1. Open this folder in VSCode
2. Click "Upload" button in PlatformIO toolbar
3. Click "Serial Monitor" to view output

### 3. Walk Around Your House

Once uploaded:
- The display will show real-time WiFi signal strength
- Walk around different areas of your house
- Watch the signal strength change
- Serial monitor logs RSSI values you can save

## Display Modes

The device automatically cycles through 3 modes every 10 seconds:

### 1. Signal Meter Mode
- Shows RSSI in dBm
- Quality percentage (0-100%)
- Visual bar graph
- Signal quality rating (Excellent/Good/Fair/Poor)

### 2. Signal Graph Mode
- Real-time scrolling graph
- Shows signal strength over time
- Great for watching changes as you move

### 3. Network Scanner Mode
- Scans for nearby WiFi networks
- Shows network names and signal strengths
- Updates every 5 seconds
- Displays up to 5 networks

## Signal Strength Guide

| RSSI (dBm) | Quality | Description |
|------------|---------|-------------|
| -30 to -50 | Excellent | Right next to router |
| -50 to -60 | Very Good | Strong signal |
| -60 to -70 | Good | Reliable connection |
| -70 to -80 | Fair | Minimum for streaming |
| -80 to -90 | Poor | Unstable connection |
| -90 to -100 | Very Poor | Nearly unusable |

## Troubleshooting

**Display shows nothing:**
- Check I2C wiring (SDA to GPIO21, SCL to GPIO22)
- Verify display address is 0x3C (some displays use 0x3D)
- Try changing `SCREEN_ADDRESS` in the code to `0x3D`

**Won't connect to WiFi:**
- Double-check SSID and password in code
- Make sure you're using 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check serial monitor for error messages

**Display is upside down:**
- Add this line after `display.begin()`:
  ```cpp
  display.setRotation(2);  // Rotate 180 degrees
  ```

**I2C pins are different:**
- Add this line in `setup()` before `display.begin()`:
  ```cpp
  Wire.begin(YOUR_SDA_PIN, YOUR_SCL_PIN);
  ```

## Future Enhancements

- [ ] Add button to manually switch modes
- [ ] Save signal data to SD card
- [ ] Web server to view data remotely
- [ ] GPS coordinates (with GPS module)
- [ ] Export data in format compatible with Python heatmap tool
- [ ] Add timestamps to measurements
- [ ] Battery power with sleep modes

## Integration with Python Heatmap Tool

To use this data with the WiFi heatmap Python tool in this repo:

1. Log serial output to a file while walking around
2. Parse RSSI values and locations
3. Convert to JSON format compatible with `wifi_measurements.json`
4. Generate heatmaps using `generate_heatmap.py`

## Serial Output Format

The device outputs to serial at 115200 baud:
```
RSSI: -65 dBm
RSSI: -68 dBm
RSSI: -71 dBm
```

You can log this while walking around and later process it.

## Pin Reference

Default ESP32 I2C pins:
- GPIO 21: SDA (Data)
- GPIO 22: SCL (Clock)
- 3.3V: Power
- GND: Ground

## License

Free to use and modify for personal and educational purposes.
