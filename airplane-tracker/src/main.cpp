#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <math.h>
#include <ESPAsyncWebServer.h>
#include "secrets.h"

// Web Server
AsyncWebServer server(80);

// OLED Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// OpenSky Network API
const char* OPENSKY_API = "https://opensky-network.org/api/states/all";

// NTP Configuration
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 7200;     // EET = +2 hours (UTC+2)
const int DAYLIGHT_OFFSET_SEC = 3600; // EEST = +3 hours in summer (UTC+3)

// Runtime-modifiable settings (initialized from secrets.h)
float currentSearchRadius = SEARCH_RADIUS_KM;
float currentMaxAltitude = MAX_ALTITUDE_M;
int currentUpdateInterval = UPDATE_INTERVAL_SEC;

// Aircraft data structure
struct Aircraft {
  String callsign;
  String icao24;
  float latitude;
  float longitude;
  float altitude;      // meters
  float velocity;      // m/s
  float heading;       // degrees
  float verticalRate;  // m/s
  float distance;      // km from observer
  bool onGround;
  unsigned long lastSeen;
  bool valid;
  String origin;       // Origin airport code
  String destination;  // Destination airport code
};

// Store up to 10 aircraft
#define MAX_AIRCRAFT 10
Aircraft aircraft[MAX_AIRCRAFT];
int aircraftCount = 0;
int currentDisplayIndex = 0;

unsigned long lastUpdate = 0;
unsigned long lastDisplayRotation = 0;
const unsigned long DISPLAY_ROTATION_INTERVAL = 5000; // 5 seconds per aircraft

// Radar animation
int radarAngle = 0;
bool isScanning = false;
unsigned long scanStartTime = 0;

// Forward declarations
void setupWebServer();
void updateAircraftData();
void fetchRouteInfo(Aircraft &plane);
void drawAircraft(Aircraft &plane);
void drawSummary();
void drawRadarScan(int angle);
float calculateDistance(float lat1, float lon1, float lat2, float lon2);
String getCompassDirection(float heading);
String formatAltitude(float meters);
String formatSpeed(float ms);
String getVerticalTrend(float rate);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("  Aircraft Overhead Tracker");
  Serial.println("  Using OpenSky Network");
  Serial.println("========================================\n");

  // Initialize I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Aircraft Tracker"));
  display.println(F("v1.0"));
  display.println();
  display.println(F("Connecting WiFi..."));
  display.display();

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    attempts++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("WiFi Connected!"));
    display.print(F("IP: "));
    display.println(WiFi.localIP());
    display.println();
    display.print(F("Location: "));
    display.println();
    display.print(F("Lat: "));
    display.println(MY_LATITUDE, 4);
    display.print(F("Lon: "));
    display.println(MY_LONGITUDE, 4);
    display.display();
    delay(3000);
    
    // Configure time
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    // Setup web server
    setupWebServer();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Web Interface:"));
    display.println(WiFi.localIP());
    display.println();
    display.println(F("Scanning for"));
    display.println(F("aircraft..."));
    display.display();
    delay(2000);

    // Initial fetch
    updateAircraftData();
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("WiFi Failed!"));
    display.println();
    display.println(F("Check secrets.h"));
    display.display();
    for(;;);
  }
}

void setupWebServer() {
  // Root page - Web interface
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Aircraft Tracker</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background: white;
      border-radius: 20px;
      padding: 30px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
    }
    h1 {
      color: #333;
      margin-bottom: 10px;
      font-size: 28px;
    }
    .subtitle {
      color: #666;
      margin-bottom: 30px;
      font-size: 14px;
    }
    .card {
      background: #f8f9fa;
      border-radius: 12px;
      padding: 20px;
      margin-bottom: 20px;
    }
    .card h2 {
      font-size: 18px;
      color: #667eea;
      margin-bottom: 15px;
    }
    .setting {
      margin-bottom: 15px;
    }
    label {
      display: block;
      font-weight: 600;
      margin-bottom: 8px;
      color: #333;
      font-size: 14px;
    }
    input[type="number"] {
      width: 100%;
      padding: 12px;
      border: 2px solid #e1e4e8;
      border-radius: 8px;
      font-size: 16px;
      transition: border 0.3s;
    }
    input[type="number"]:focus {
      outline: none;
      border-color: #667eea;
    }
    button {
      width: 100%;
      padding: 15px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s, box-shadow 0.2s;
      margin-top: 10px;
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 10px 20px rgba(102, 126, 234, 0.4);
    }
    button:active {
      transform: translateY(0);
    }
    .info {
      background: #e3f2fd;
      padding: 15px;
      border-radius: 8px;
      margin-bottom: 20px;
      border-left: 4px solid #2196f3;
    }
    .info p {
      margin: 5px 0;
      font-size: 14px;
      color: #1976d2;
    }
    .status {
      text-align: center;
      padding: 10px;
      border-radius: 8px;
      margin-top: 15px;
      display: none;
      font-weight: 600;
    }
    .status.success {
      background: #d4edda;
      color: #155724;
      display: block;
    }
    .status.error {
      background: #f8d7da;
      color: #721c24;
      display: block;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>✈️ Aircraft Tracker</h1>
    <p class="subtitle">Configure your aircraft tracking settings</p>

    <div class="info">
      <p><strong>IP Address:</strong> )rawliteral" + WiFi.localIP().toString() + R"rawliteral(</p>
      <p><strong>Status:</strong> <span id="aircraft-count">Loading...</span></p>
    </div>

    <div class="card">
      <h2>📡 Search Settings</h2>
      <div class="setting">
        <label for="radius">Search Radius (km)</label>
        <input type="number" id="radius" value=")rawliteral" + String((int)currentSearchRadius) + R"rawliteral(" min="1" max="250" step="5">
      </div>
      <div class="setting">
        <label for="altitude">Max Altitude (meters)</label>
        <input type="number" id="altitude" value=")rawliteral" + String((int)currentMaxAltitude) + R"rawliteral(" min="500" max="15000" step="500">
      </div>
      <div class="setting">
        <label for="interval">Update Interval (seconds)</label>
        <input type="number" id="interval" value=")rawliteral" + String(currentUpdateInterval) + R"rawliteral(" min="10" max="120" step="5">
      </div>
      <button onclick="saveSettings()">💾 Save Settings</button>
    </div>

    <div class="card">
      <h2>🎯 Quick Actions</h2>
      <button onclick="updateNow()">🔄 Update Now</button>
      <button onclick="getStatus()">📊 Get Status</button>
    </div>

    <div id="status" class="status"></div>
  </div>

  <script>
    function saveSettings() {
      const radius = document.getElementById('radius').value;
      const altitude = document.getElementById('altitude').value;
      const interval = document.getElementById('interval').value;

      fetch('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({radius, altitude, interval})
      })
      .then(r => r.json())
      .then(data => {
        showStatus(data.message, 'success');
      })
      .catch(err => {
        showStatus('Failed to save settings', 'error');
      });
    }

    function updateNow() {
      fetch('/api/update')
      .then(r => r.json())
      .then(data => {
        showStatus(data.message, 'success');
        setTimeout(getStatus, 2000);
      });
    }

    function getStatus() {
      fetch('/api/status')
      .then(r => r.json())
      .then(data => {
        document.getElementById('aircraft-count').textContent =
          data.aircraft + ' aircraft detected';
      });
    }

    function showStatus(msg, type) {
      const status = document.getElementById('status');
      status.textContent = msg;
      status.className = 'status ' + type;
      setTimeout(() => {
        status.className = 'status';
      }, 3000);
    }

    // Auto-update status every 5 seconds
    setInterval(getStatus, 5000);
    getStatus();
  </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html; charset=UTF-8", html);
  });

  // API endpoint: Get status
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["aircraft"] = aircraftCount;
    doc["radius"] = currentSearchRadius;
    doc["altitude"] = currentMaxAltitude;
    doc["interval"] = currentUpdateInterval;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API endpoint: Update now
  server.on("/api/update", HTTP_GET, [](AsyncWebServerRequest *request){
    updateAircraftData();
    JsonDocument doc;
    doc["message"] = "Update triggered successfully";
    doc["aircraft"] = aircraftCount;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API endpoint: Save settings
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      JsonDocument doc;
      deserializeJson(doc, (const char*)data);

      // Update runtime settings
      if(doc.containsKey("radius")) {
        currentSearchRadius = doc["radius"].as<float>();
        Serial.print("Search radius updated to: ");
        Serial.println(currentSearchRadius);
      }
      if(doc.containsKey("altitude")) {
        currentMaxAltitude = doc["altitude"].as<float>();
        Serial.print("Max altitude updated to: ");
        Serial.println(currentMaxAltitude);
      }
      if(doc.containsKey("interval")) {
        currentUpdateInterval = doc["interval"].as<int>();
        Serial.print("Update interval updated to: ");
        Serial.println(currentUpdateInterval);
      }

      JsonDocument response;
      response["message"] = "Settings applied immediately!";
      String responseStr;
      serializeJson(response, responseStr);
      request->send(200, "application/json", responseStr);
    });

  server.begin();
  Serial.println("Web server started!");
  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println(" in your browser");
}

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  // Haversine formula for distance calculation
  const float R = 6371.0; // Earth radius in km
  
  float dLat = (lat2 - lat1) * PI / 180.0;
  float dLon = (lon2 - lon1) * PI / 180.0;
  
  float a = sin(dLat/2) * sin(dLat/2) +
            cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) *
            sin(dLon/2) * sin(dLon/2);
  
  float c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

String getCompassDirection(float heading) {
  if(heading < 0) return "?";
  
  if(heading >= 337.5 || heading < 22.5) return "N";
  if(heading >= 22.5 && heading < 67.5) return "NE";
  if(heading >= 67.5 && heading < 112.5) return "E";
  if(heading >= 112.5 && heading < 157.5) return "SE";
  if(heading >= 157.5 && heading < 202.5) return "S";
  if(heading >= 202.5 && heading < 247.5) return "SW";
  if(heading >= 247.5 && heading < 292.5) return "W";
  if(heading >= 292.5 && heading < 337.5) return "NW";
  return "?";
}

String formatAltitude(float meters) {
  if(meters < 0) return "Ground";
  float feet = meters * 3.28084;
  if(feet < 1000) return String((int)feet) + "ft";
  return String(feet / 1000.0, 1) + "Kft";
}

String formatSpeed(float ms) {
  if(ms < 0) return "N/A";
  float knots = ms * 1.94384;
  return String((int)knots) + "kts";
}

String getVerticalTrend(float rate) {
  if(rate > 2.0) return "^^";
  if(rate > 0.5) return "^";
  if(rate < -2.0) return "vv";
  if(rate < -0.5) return "v";
  return "--";
}

void fetchRouteInfo(Aircraft &plane) {
  // Try to fetch route information from AeroDataBox API (free tier)
  // Note: This is best-effort - may not always return data
  if(plane.callsign.length() == 0) return;

  HTTPClient http;
  http.setTimeout(5000);

  // Clean callsign (remove whitespace)
  String cleanCallsign = plane.callsign;
  cleanCallsign.trim();
  if(cleanCallsign.length() == 0) return;

  // Try FlightAware's free endpoint (no API key needed for basic info)
  String url = "https://opensky-network.org/api/routes?callsign=" + cleanCallsign;

  Serial.print("[Route] Fetching route for: ");
  Serial.println(cleanCallsign);

  http.begin(url);
  int httpCode = http.GET();

  if(httpCode == 200) {
    String payload = http.getString();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if(!error) {
      // OpenSky routes API returns: {"route": ["AIRPORT1", "AIRPORT2", ...]}
      if(doc.containsKey("route") && doc["route"].is<JsonArray>()) {
        JsonArray route = doc["route"].as<JsonArray>();
        if(route.size() >= 2) {
          plane.origin = route[0].as<String>();
          plane.destination = route[route.size() - 1].as<String>();
          Serial.print("[Route] Found: ");
          Serial.print(plane.origin);
          Serial.print(" -> ");
          Serial.println(plane.destination);
        }
      }
    }
  } else {
    Serial.print("[Route] HTTP error: ");
    Serial.println(httpCode);
  }

  http.end();
}

void updateAircraftData() {
  if(WiFi.status() != WL_CONNECTED) return;

  Serial.println("\n[OpenSky] Fetching aircraft data...");

  HTTPClient http;
  http.setTimeout(20000);
  
  // Calculate bounding box (roughly square around location)
  float latDelta = currentSearchRadius / 111.0; // ~111 km per degree latitude
  float lonDelta = currentSearchRadius / (111.0 * cos(MY_LATITUDE * PI / 180.0));
  
  float minLat = MY_LATITUDE - latDelta;
  float maxLat = MY_LATITUDE + latDelta;
  float minLon = MY_LONGITUDE - lonDelta;
  float maxLon = MY_LONGITUDE + lonDelta;
  
  String url = String(OPENSKY_API) + 
               "?lamin=" + String(minLat, 4) +
               "&lomin=" + String(minLon, 4) +
               "&lamax=" + String(maxLat, 4) +
               "&lomax=" + String(maxLon, 4);
  
  Serial.println("API URL: " + url);
  
  http.begin(url);
  int httpCode = http.GET();
  
  if(httpCode == 200) {
    String payload = http.getString();
    Serial.printf("Response size: %d bytes\n", payload.length());
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if(!error) {
      JsonArray states = doc["states"];
      
      if(states.isNull() || states.size() == 0) {
        Serial.println("No aircraft found in area");
        aircraftCount = 0;
      } else {
        Serial.printf("Found %d aircraft\n", states.size());
        
        aircraftCount = 0;
        
        for(JsonArray state : states) {
          if(aircraftCount >= MAX_AIRCRAFT) break;
          
          // Parse state vector
          String icao24 = state[0] | "";
          String callsign = state[1] | "";
          float lon = state[5] | 0.0;
          float lat = state[6] | 0.0;
          float altitude = state[7] | -1.0;
          bool onGround = state[8] | false;
          float velocity = state[9] | -1.0;
          float heading = state[10] | -1.0;
          float vertRate = state[11] | 0.0;
          
          // Skip if no position data
          if(lat == 0.0 && lon == 0.0) continue;
          
          // Calculate distance
          float dist = calculateDistance(MY_LATITUDE, MY_LONGITUDE, lat, lon);
          
          // Filter by altitude (if airborne)
          if(!onGround && altitude > currentMaxAltitude) continue;
          
          // Store aircraft
          aircraft[aircraftCount].icao24 = icao24;
          aircraft[aircraftCount].callsign = callsign;
          aircraft[aircraftCount].callsign.trim();
          aircraft[aircraftCount].latitude = lat;
          aircraft[aircraftCount].longitude = lon;
          aircraft[aircraftCount].altitude = altitude;
          aircraft[aircraftCount].velocity = velocity;
          aircraft[aircraftCount].heading = heading;
          aircraft[aircraftCount].verticalRate = vertRate;
          aircraft[aircraftCount].distance = dist;
          aircraft[aircraftCount].onGround = onGround;
          aircraft[aircraftCount].valid = true;
          aircraft[aircraftCount].origin = "";
          aircraft[aircraftCount].destination = "";

          Serial.printf("  [%d] %s @ %s, %.1f km, %s\n",
                        aircraftCount,
                        aircraft[aircraftCount].callsign.c_str(),
                        formatAltitude(altitude).c_str(),
                        dist,
                        onGround ? "Ground" : "Airborne");
          
          aircraftCount++;
        }
        
        // Sort by distance (closest first)
        for(int i = 0; i < aircraftCount - 1; i++) {
          for(int j = i + 1; j < aircraftCount; j++) {
            if(aircraft[j].distance < aircraft[i].distance) {
              Aircraft temp = aircraft[i];
              aircraft[i] = aircraft[j];
              aircraft[j] = temp;
            }
          }
        }

        Serial.printf("Tracking %d aircraft\n", aircraftCount);

        // Fetch route info for the closest aircraft (if any)
        if(aircraftCount > 0) {
          fetchRouteInfo(aircraft[0]);
        }
      }
    } else {
      Serial.println("[OpenSky] JSON parse error!");
      Serial.println(error.c_str());
    }
  } else {
    Serial.printf("[OpenSky] HTTP error: %d\n", httpCode);
    if(httpCode == 429) {
      Serial.println("Rate limited - will retry later");
    }
  }
  
  http.end();
  lastUpdate = millis();
}

void drawRadarScan(int angle) {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("SCANNING"));

  // Time
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    display.setCursor(98, 0);
    display.print(timeStr);
  }

  display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);

  // Radar circle center and radius
  int centerX = 64;
  int centerY = 38;
  int radius = 22;

  // Draw static radar circles (3 rings)
  display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);
  display.drawCircle(centerX, centerY, radius * 2 / 3, SSD1306_WHITE);
  display.drawCircle(centerX, centerY, radius / 3, SSD1306_WHITE);

  // Draw crosshairs
  display.drawLine(centerX - radius, centerY, centerX + radius, centerY, SSD1306_WHITE);
  display.drawLine(centerX, centerY - radius, centerX, centerY + radius, SSD1306_WHITE);

  // Draw center dot
  display.fillCircle(centerX, centerY, 2, SSD1306_WHITE);

  // Range info (bottom) - now shows current search radius
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print(F("Range: "));
  if(currentSearchRadius >= 1.0) {
    display.print((int)currentSearchRadius);
    display.print(F("km"));
  } else {
    display.print((int)(currentSearchRadius * 1000));
    display.print(F("m"));
  }

  display.display();
}

void drawSummary() {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("AIRCRAFT NEARBY"));
  
  // Time
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    display.setCursor(98, 0);
    display.print(timeStr);
  }
  
  display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);
  
  if(aircraftCount == 0) {
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println(F("No aircraft"));
    display.println(F("detected"));
    display.println();
    display.print(F("Range "));
    display.print(currentSearchRadius, 0);
    display.println(F("km"));
    display.print(F("Alt: "));
    display.print(formatAltitude(currentMaxAltitude));
  } else {
    // Aircraft count
    display.setCursor(0, 12);
    display.setTextSize(1);
    display.print(aircraftCount);
    display.print(F(" aircraft found"));
    
    // Show closest 3
    int showCount = min(aircraftCount, 3);
    for(int i = 0; i < showCount; i++) {
      int y = 24 + (i * 13);
      
      display.setCursor(0, y);
      display.setTextSize(1);
      
      // Callsign or ICAO
      String id = aircraft[i].callsign;
      if(id.length() == 0) id = aircraft[i].icao24;
      if(id.length() > 8) id = id.substring(0, 8);
      display.print(id);
      
      // Distance
      display.setCursor(60, y);
      display.print(aircraft[i].distance, 1);
      display.print(F("km"));
      
      // Altitude
      display.setCursor(0, y + 8);
      display.print(formatAltitude(aircraft[i].altitude));
      
      // Direction
      display.setCursor(50, y + 8);
      display.print(getCompassDirection(aircraft[i].heading));
      
      // Speed
      display.setCursor(70, y + 8);
      display.print(formatSpeed(aircraft[i].velocity));
    }
    
    if(aircraftCount > 3) {
      display.setCursor(0, 60);
      display.print(F("+ "));
      display.print(aircraftCount - 3);
      display.print(F(" more"));
    }
  }
  
  display.display();
}

void drawAircraft(Aircraft &plane) {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("OVERHEAD"));

  // Time
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    display.setCursor(98, 0);
    display.print(timeStr);
  }

  display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);

  // Flight Number / Callsign - Large
  String callsign = plane.callsign;
  if(callsign.length() == 0) {
    callsign = plane.icao24; // Fallback to ICAO24 hex
  }

  display.setTextSize(2);
  display.setCursor(0, 11);
  if(callsign.length() > 8) callsign = callsign.substring(0, 8);
  display.print(callsign);

  // Route (Origin -> Destination)
  display.setTextSize(1);
  display.setCursor(0, 27);
  if(plane.origin.length() > 0 && plane.destination.length() > 0) {
    display.print(plane.origin);
    display.print(F(" -> "));
    display.print(plane.destination);
  }

  // Altitude in feet
  display.setCursor(0, 36);
  display.print(F("Alt: "));
  if(plane.altitude >= 0) {
    float altFeet = plane.altitude * 3.28084;
    display.print((int)altFeet);
    display.print(F("ft"));

    // Vertical trend
    display.print(F(" "));
    display.print(getVerticalTrend(plane.verticalRate));
  } else {
    display.print(F("GROUND"));
  }

  // Airspeed in knots
  display.setCursor(0, 45);
  display.print(F("Spd: "));
  if(plane.velocity >= 0) {
    float knots = plane.velocity * 1.94384;
    display.print((int)knots);
    display.print(F("kts"));
  } else {
    display.print(F("N/A"));
  }

  // Heading (degrees and compass direction)
  display.setCursor(0, 54);
  display.print(F("Hdg: "));
  if(plane.heading >= 0) {
    display.print((int)plane.heading);
    display.print(F("° "));
    display.print(getCompassDirection(plane.heading));
  } else {
    display.print(F("N/A"));
  }

  display.display();
}

void loop() {
  unsigned long currentMillis = millis();

  // Update aircraft data periodically (includes radar animation)
  if(currentMillis - lastUpdate >= (currentUpdateInterval * 1000)) {
    updateAircraftData();
  }

  // Display logic
  if(aircraftCount == 0) {
    // No aircraft - show static radar display
    static unsigned long lastRadarDraw = 0;
    if(currentMillis - lastRadarDraw >= 1000) { // Update every second
      drawRadarScan(0); // Static display (angle parameter not used anymore)
      lastRadarDraw = currentMillis;
    }
    delay(100);
  } else {
    // Show closest aircraft continuously (already sorted by distance)
    if(currentMillis - lastDisplayRotation >= 500) { // Refresh every 0.5s
      drawAircraft(aircraft[0]); // Index 0 is always closest
      lastDisplayRotation = currentMillis;
    }
    delay(100);
  }
}
