#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

// OLED Display configuration for Heltec WiFi Kit 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 4      // Heltec uses GPIO 4 for SDA
#define OLED_SCL 15     // Heltec uses GPIO 15 for SCL
#define OLED_RST 16     // Heltec uses GPIO 16 for RST
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Dashboard display modes
enum DisplayMode {
  MODE_OVERVIEW,
  MODE_WIFI_DETAILS,
  MODE_MEMORY,
  MODE_TEMPERATURE,
  MODE_SYSTEM_INFO
};

DisplayMode currentMode = MODE_OVERVIEW;
unsigned long lastModeChange = 0;
const unsigned long MODE_DURATION = 5000; // 5 seconds per mode

// Temperature reading (ESP32 internal sensor)
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

// Function to get ESP32 internal temperature
float getInternalTemperature() {
  return (temprature_sens_read() - 32) / 1.8;
}

// Function to get WiFi signal quality percentage
int getWiFiQuality(int rssi) {
  if(rssi >= -50) return 100;
  else if(rssi >= -60) return 90;
  else if(rssi >= -70) return 80;
  else if(rssi >= -80) return 60;
  else if(rssi >= -90) return 40;
  else return 20;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32 Sensor Dashboard Starting...");
  Serial.println("=================================");

  // Initialize I2C with Heltec WiFi Kit 32 pins
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("I2C initialized");

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  Serial.println("Display initialized successfully!");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("ESP32 Dashboard"));
  display.println(F("Initializing..."));
  display.display();

  delay(2000);

  // Start WiFi in station mode (but don't connect to any network)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Sensor Dashboard Ready!");
}

void drawOverview() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  // Title
  display.println(F("== SENSOR OVERVIEW =="));
  
  // CPU Frequency
  display.print(F("CPU: "));
  display.print(getCpuFrequencyMhz());
  display.println(F(" MHz"));
  
  // Free Heap Memory
  display.print(F("Heap: "));
  display.print(ESP.getFreeHeap() / 1024);
  display.println(F(" KB"));
  
  // Temperature
  float temp = getInternalTemperature();
  display.print(F("Temp: "));
  display.print(temp, 1);
  display.println(F(" C"));
  
  // WiFi Networks
  int networks = WiFi.scanNetworks();
  display.print(F("WiFi: "));
  display.print(networks);
  display.println(F(" networks"));
  
  // Uptime
  unsigned long uptime = millis() / 1000;
  display.print(F("Uptime: "));
  if(uptime > 3600) {
    display.print(uptime / 3600);
    display.print(F("h "));
    display.print((uptime % 3600) / 60);
    display.print(F("m"));
  } else if(uptime > 60) {
    display.print(uptime / 60);
    display.print(F("m "));
    display.print(uptime % 60);
    display.print(F("s"));
  } else {
    display.print(uptime);
    display.print(F(" seconds"));
  }
  
  display.display();
}

void drawWiFiDetails() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  display.println(F("=== WiFi SCAN ==="));
  
  int networks = WiFi.scanNetworks();
  
  if(networks == 0) {
    display.println(F("No networks found"));
  } else {
    display.print(F("Found: "));
    display.print(networks);
    display.println(F(" networks"));
    display.println();
    
    // Show top 3 strongest networks
    int count = min(networks, 3);
    for(int i = 0; i < count; i++) {
      String ssid = WiFi.SSID(i);
      if(ssid.length() > 14) {
        ssid = ssid.substring(0, 13) + "~";
      }
      display.print(ssid);
      
      int rssi = WiFi.RSSI(i);
      display.print(F(" "));
      display.print(rssi);
      display.println(F("dB"));
    }
  }
  
  display.display();
}

void drawMemoryDetails() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  display.println(F("==== MEMORY ===="));
  display.println();
  
  // Heap Memory
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t heapSize = ESP.getHeapSize();
  uint32_t usedHeap = heapSize - freeHeap;
  
  display.print(F("Total: "));
  display.print(heapSize / 1024);
  display.println(F(" KB"));
  
  display.print(F("Free:  "));
  display.print(freeHeap / 1024);
  display.println(F(" KB"));
  
  display.print(F("Used:  "));
  display.print(usedHeap / 1024);
  display.println(F(" KB"));
  
  // Memory usage bar
  int barWidth = map(usedHeap, 0, heapSize, 0, SCREEN_WIDTH);
  display.println();
  display.drawRect(0, 50, SCREEN_WIDTH, 10, SSD1306_WHITE);
  display.fillRect(0, 50, barWidth, 10, SSD1306_WHITE);
  
  display.display();
}

void drawTemperatureDetails() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  display.println(F("== TEMPERATURE =="));
  display.println();
  
  float temp = getInternalTemperature();
  
  // Large temperature display
  display.setTextSize(3);
  display.setCursor(10, 20);
  display.print(temp, 1);
  
  display.setTextSize(1);
  display.setCursor(95, 22);
  display.print(F("o"));
  display.setCursor(100, 27);
  display.print(F("C"));
  
  // Status
  display.setCursor(0, 50);
  if(temp < 50) {
    display.print(F("Status: NORMAL"));
  } else if(temp < 70) {
    display.print(F("Status: WARM"));
  } else {
    display.print(F("Status: HOT!"));
  }
  
  display.display();
}

void drawSystemInfo() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  display.println(F("=== SYSTEM INFO ==="));
  
  // Chip model
  display.print(F("Chip: "));
  display.println(ESP.getChipModel());
  
  // Number of cores
  display.print(F("Cores: "));
  display.println(ESP.getChipCores());
  
  // CPU Frequency
  display.print(F("CPU: "));
  display.print(getCpuFrequencyMhz());
  display.println(F(" MHz"));
  
  // Flash size
  display.print(F("Flash: "));
  display.print(ESP.getFlashChipSize() / (1024 * 1024));
  display.println(F(" MB"));
  
  // SDK Version
  display.print(F("SDK: "));
  display.println(ESP.getSdkVersion());
  
  display.display();
}

void loop() {
  // Auto-cycle through display modes
  if(millis() - lastModeChange > MODE_DURATION) {
    currentMode = (DisplayMode)((currentMode + 1) % 5);
    lastModeChange = millis();
    
    // Print mode change to serial
    Serial.print("Mode: ");
    switch(currentMode) {
      case MODE_OVERVIEW: Serial.println("Overview"); break;
      case MODE_WIFI_DETAILS: Serial.println("WiFi Details"); break;
      case MODE_MEMORY: Serial.println("Memory"); break;
      case MODE_TEMPERATURE: Serial.println("Temperature"); break;
      case MODE_SYSTEM_INFO: Serial.println("System Info"); break;
    }
  }
  
  // Display current mode
  switch(currentMode) {
    case MODE_OVERVIEW:
      drawOverview();
      break;
      
    case MODE_WIFI_DETAILS:
      drawWiFiDetails();
      break;
      
    case MODE_MEMORY:
      drawMemoryDetails();
      break;
      
    case MODE_TEMPERATURE:
      drawTemperatureDetails();
      break;
      
    case MODE_SYSTEM_INFO:
      drawSystemInfo();
      break;
  }
  
  delay(100);
}
