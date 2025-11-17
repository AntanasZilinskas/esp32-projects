# Aircraft Overhead Tracker

Real-time aircraft tracking system using ESP32 and OpenSky Network API. Displays aircraft flying overhead with detailed information on an OLED screen.

## Features

- **Live Aircraft Tracking**: Shows planes within your configurable search radius
- **Detailed Information**: Callsign, altitude, speed, heading, distance
- **Auto-Sorting**: Displays closest aircraft first
- **Multiple Views**: Summary screen + individual aircraft details
- **Smart Filtering**: Configure max altitude and search radius
- **Direction Indicators**: Compass headings and vertical trends
- **No API Key Required**: Uses free OpenSky Network API

## Display Modes

### Summary Screen
Shows overview of all detected aircraft:
```
AIRCRAFT NEARBY    12:45
───────────────────────
3 aircraft found

UAL1234    2.3km
12.5Kft NE 425kts

DAL892     4.1km
8.2Kft  SW 380kts

AAL567     5.8km
15.1Kft N  450kts
```

### Individual Aircraft Screen
Detailed view for each plane:
```
AIRCRAFT 1/3      12:45
───────────────────────
UAL1234

2.3 km

Alt: 12.5Kft ^^
Spd: 425kts
Hdg: 285° NW
```

## Hardware Requirements

- **ESP32** (Heltec WiFi Kit 32 or similar)
- **OLED Display** (SSD1306, 128x64 pixels)
- **WiFi Connection**
- **Power Supply** (USB)

## Setup Instructions

### 1. Configure Your Location

Edit `src/secrets.h` with your coordinates:

```cpp
const float MY_LATITUDE = 40.7128;    // Your latitude
const float MY_LONGITUDE = -74.0060;  // Your longitude
```

Find your coordinates:
1. Go to [Google Maps](https://www.google.com/maps)
2. Right-click on your location
3. Click "What's here?"
4. Copy the coordinates

### 2. Configure WiFi

Already in `src/secrets.h`:

```cpp
const char* WIFI_SSID = "YourWiFi";
const char* WIFI_PASSWORD = "YourPassword";
```

### 3. Adjust Search Settings (Optional)

```cpp
const float SEARCH_RADIUS_KM = 25.0;    // How far to search
const float MAX_ALTITUDE_M = 5000.0;    // Max altitude (~16,400 ft)
const int UPDATE_INTERVAL_SEC = 15;     // Update frequency
```

### 4. Build and Upload

```bash
cd airplane-tracker
python3 -m platformio run --target upload
```

### 5. Deploy

Plug into any USB power source and it will:
1. Connect to WiFi
2. Sync time
3. Start scanning for aircraft
4. Display results automatically

## How It Works

### Data Source
- **OpenSky Network**: Free, community-driven ADS-B data
- **Coverage**: Worldwide (where volunteers have receivers)
- **Update Rate**: Real-time (15-second refresh default)
- **No Authentication**: Public API, no account needed

### Detection Method
1. Queries OpenSky API with your GPS coordinates
2. Gets all aircraft in bounding box (defined by search radius)
3. Filters by altitude (ignores high-altitude flights if configured)
4. Calculates distance from your location
5. Sorts by distance (closest first)
6. Displays up to 10 aircraft

### Information Displayed

**For Each Aircraft:**
- **Callsign**: Flight number (e.g., UAL1234) or ICAO24 hex code
- **Distance**: Straight-line distance in kilometers
- **Altitude**: Height in feet (K = thousands)
- **Speed**: Ground speed in knots
- **Heading**: Direction (0-360° + compass direction)
- **Vertical Rate**: Climbing (^^, ^) / Level (--) / Descending (v, vv)
- **Ground Status**: Shows "GROUND" if parked/taxiing

## Display Indicators

### Vertical Trends
- `^^` - Climbing fast (>2 m/s)
- `^` - Climbing (>0.5 m/s)
- `--` - Level flight
- `v` - Descending (<-0.5 m/s)
- `vv` - Descending fast (<-2 m/s)

### Compass Directions
- `N`, `NE`, `E`, `SE`, `S`, `SW`, `W`, `NW`

## Customization

### Change Update Frequency
```cpp
const int UPDATE_INTERVAL_SEC = 15;  // Faster = 10, Slower = 30
```
Note: OpenSky API has rate limits (anonymous: ~100 requests/day)

### Adjust Search Area
```cpp
const float SEARCH_RADIUS_KM = 25.0;  // Increase to see more distant aircraft
```

### Filter by Altitude
```cpp
const float MAX_ALTITUDE_M = 5000.0;  // Only show aircraft below this height
```
- 5000m = ~16,400 ft (low-flying aircraft)
- 10000m = ~32,800 ft (most commercial flights)
- Set very high to see all aircraft

### Display Rotation Speed
In `main.cpp`:
```cpp
const unsigned long DISPLAY_ROTATION_INTERVAL = 5000;  // milliseconds
```

## Troubleshooting

### No Aircraft Detected
- **Check your location**: Ensure coordinates are correct
- **Increase search radius**: Try 50km or more
- **Raise altitude limit**: Commercial flights often cruise at 30,000+ ft
- **Check coverage**: Rural areas may have sparse ADS-B coverage
- **Wait**: Aircraft may not always be overhead

### "HTTP Error 429"
- **Rate Limited**: OpenSky API limits anonymous requests
- **Solution**: Increase `UPDATE_INTERVAL_SEC` to 30 or 60
- **Alternative**: Create free OpenSky account for higher limits

### WiFi Connection Failed
- Check SSID and password in `secrets.h`
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Move closer to router

### Incomplete Data
- Some aircraft may not broadcast all information
- Older aircraft may lack ADS-B entirely
- Military aircraft often don't appear in public feeds

## Performance

- **Memory Usage**: ~15-16% RAM
- **Update Interval**: 15 seconds (configurable)
- **Max Aircraft Tracked**: 10 simultaneously
- **API Response Time**: 2-5 seconds typical
- **Display Refresh**: 5 seconds per aircraft

## Privacy & Security

- **Your location**: Stored locally only, sent to OpenSky API for queries
- **WiFi credentials**: Gitignored, never transmitted
- **No personal data**: Only receives public aircraft transponder data
- **Read-only**: Device doesn't transmit anything except API requests

## API Information

### OpenSky Network
- **Free Tier**: ~100 requests/day (anonymous)
- **Rate Limit**: Relaxed for reasonable use
- **Coverage**: Global ADS-B data
- **Documentation**: https://openskynetwork.github.io/opensky-api/

### Data Freshness
- Aircraft positions updated every 10-15 seconds
- Your device updates every 15 seconds (configurable)
- Total latency: Typically <30 seconds

## Future Enhancements

Potential additions:
- Button controls to pause/filter
- Historical tracking (flight paths)
- Aircraft type lookup
- Airline logo display
- Sound notifications
- Speed trend indicators
- Multiple location profiles
- Export flight logs

## About ADS-B

**Automatic Dependent Surveillance-Broadcast (ADS-B):**
- Modern aircraft broadcast their position, altitude, speed
- Required in most airspace since 2020
- Public, unencrypted transmissions on 1090 MHz
- Received by ground stations and shared via OpenSky Network
- More accurate than radar

## Credits

- **OpenSky Network**: https://opensky-network.org
- **Adafruit Libraries**: Display drivers
- **ArduinoJson**: JSON parsing

## License

MIT License - Free to use and modify!
