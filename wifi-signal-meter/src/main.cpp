#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display configuration for Heltec WiFi Kit 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 4      // Heltec uses GPIO 4 for SDA
#define OLED_SCL 15     // Heltec uses GPIO 15 for SCL
#define OLED_RST 16     // Heltec uses GPIO 16 for RST
#define SCREEN_ADDRESS 0x3C  // Common I2C address for SSD1306

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// WiFi configuration - EDIT THESE
const char* WIFI_SSID = "pilot f-16";  // Change to your WiFi name
const char* WIFI_PASSWORD = "kalakutas123";  // Change to your WiFi password

// Display modes
enum DisplayMode {
  MODE_SIGNAL_METER,
  MODE_SIGNAL_GRAPH,
  MODE_NETWORK_SCANNER
};

DisplayMode currentMode = MODE_SIGNAL_METER;

// For signal strength history (graph)
#define HISTORY_SIZE 120
int signalHistory[HISTORY_SIZE];
int historyIndex = 0;

// For network scanner
int networkCount = 0;
int selectedNetwork = 0;
unsigned long lastScanTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=================================");
  Serial.println("WiFi Signal Strength Meter Starting...");
  Serial.println("=================================");

  // Initialize I2C with Heltec WiFi Kit 32 pins
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("I2C initialized on Heltec WiFi Kit 32 pins (SDA=4, SCL=15)");

  // Scan for I2C devices
  Serial.println("Scanning for I2C devices...");
  byte count = 0;
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at address 0x");
      if (i < 16) Serial.print("0");
      Serial.println(i, HEX);
      count++;
    }
  }
  if (count == 0) {
    Serial.println("No I2C devices found!");
    Serial.println("This is unusual for Heltec WiFi Kit 32 - the OLED should be built-in!");
    Serial.println("\nWill try to initialize display anyway...");
  } else {
    Serial.print("Found ");
    Serial.print(count);
    Serial.println(" I2C device(s) - Good!");
  }

  // Initialize OLED display
  Serial.print("\nAttempting to initialize display at address 0x");
  Serial.println(SCREEN_ADDRESS, HEX);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("\n*** SSD1306 INITIALIZATION FAILED! ***"));
    Serial.println(F("The display library could not communicate with the OLED."));
    Serial.println(F("\nPossible fixes:"));
    Serial.println(F("1. Check wiring - make sure display is firmly connected"));
    Serial.println(F("2. Try swapping SDA and SCL pins"));
    Serial.println(F("3. Try changing SCREEN_ADDRESS to 0x3D in code"));
    Serial.println(F("4. Make sure display has power (VCC connected)"));
    Serial.println(F("\nStopping here - display is required."));
    for(;;) {
      delay(5000);
      Serial.println("Still waiting for display...");
    }
  }

  Serial.println("\n*** Display initialized successfully! ***");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("WiFi Signal Meter"));
  display.println(F("Initializing..."));
  display.display();

  // Initialize signal history
  for(int i = 0; i < HISTORY_SIZE; i++) {
    signalHistory[i] = -100;
  }

  delay(2000);

  // Connect to WiFi
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Connecting to:"));
  display.println(WIFI_SSID);
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;

    display.print(".");
    display.display();
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Connected!"));
    display.print(F("IP: "));
    display.println(WiFi.localIP());
    display.display();
    delay(2000);
  } else {
    Serial.println("\nFailed to connect!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Connection Failed!"));
    display.println(F("Check credentials"));
    display.display();
    delay(3000);
  }
}

void drawSignalMeter(int rssi) {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("WiFi Signal Meter"));

  // Draw RSSI value
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.print(rssi);
  display.setTextSize(1);
  display.println(F(" dBm"));

  // Signal quality percentage
  int quality = 0;
  if(rssi >= -50) quality = 100;
  else if(rssi >= -60) quality = 90;
  else if(rssi >= -70) quality = 80;
  else if(rssi >= -80) quality = 60;
  else if(rssi >= -90) quality = 40;
  else quality = 20;

  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print(F("Quality: "));
  display.print(quality);
  display.println(F("%"));

  // Draw signal bar graph
  int barWidth = map(quality, 0, 100, 0, SCREEN_WIDTH);
  display.fillRect(0, 50, barWidth, 10, SSD1306_WHITE);
  display.drawRect(0, 50, SCREEN_WIDTH, 10, SSD1306_WHITE);

  // Signal quality indicator
  display.setCursor(0, 45);
  if(quality >= 80) display.print(F("Excellent"));
  else if(quality >= 60) display.print(F("Good"));
  else if(quality >= 40) display.print(F("Fair"));
  else display.print(F("Poor"));

  display.display();
}

void drawSignalGraph(int rssi) {
  // Add current signal to history
  signalHistory[historyIndex] = rssi;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Signal: "));
  display.print(rssi);
  display.println(F(" dBm"));

  // Draw graph
  int graphHeight = 48;
  int graphY = 15;

  // Draw axes
  display.drawLine(0, graphY + graphHeight, SCREEN_WIDTH, graphY + graphHeight, SSD1306_WHITE);

  // Draw signal history
  for(int i = 1; i < HISTORY_SIZE && i < SCREEN_WIDTH; i++) {
    int idx1 = (historyIndex + i - 1) % HISTORY_SIZE;
    int idx2 = (historyIndex + i) % HISTORY_SIZE;

    if(signalHistory[idx1] > -100 && signalHistory[idx2] > -100) {
      // Map RSSI (-100 to -30) to graph height
      int y1 = map(signalHistory[idx1], -100, -30, graphHeight, 0);
      int y2 = map(signalHistory[idx2], -100, -30, graphHeight, 0);

      display.drawLine(i-1, graphY + y1, i, graphY + y2, SSD1306_WHITE);
    }
  }

  // Draw reference lines
  display.setTextSize(1);
  int y_excellent = map(-50, -100, -30, graphHeight, 0);
  display.drawLine(0, graphY + y_excellent, 5, graphY + y_excellent, SSD1306_WHITE);

  display.display();
}

void drawNetworkScanner() {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Networks: "));
  display.println(networkCount);

  // Scan for networks every 5 seconds
  if(millis() - lastScanTime > 5000) {
    networkCount = WiFi.scanNetworks();
    lastScanTime = millis();
  }

  // Display up to 5 networks
  int displayCount = min(networkCount, 5);
  for(int i = 0; i < displayCount; i++) {
    int y = 12 + (i * 10);

    // Highlight selected network
    if(i == selectedNetwork % displayCount) {
      display.fillRect(0, y-1, SCREEN_WIDTH, 9, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.setCursor(0, y);

    // Network name (truncate if too long)
    String ssid = WiFi.SSID(i);
    if(ssid.length() > 12) {
      ssid = ssid.substring(0, 11) + "~";
    }
    display.print(ssid);

    // Signal strength
    int rssi = WiFi.RSSI(i);
    display.setCursor(90, y);
    display.print(rssi);
    display.print(F("dB"));
  }

  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void loop() {
  // Get current WiFi signal strength
  int rssi = WiFi.RSSI();

  // Display based on current mode
  switch(currentMode) {
    case MODE_SIGNAL_METER:
      drawSignalMeter(rssi);
      break;

    case MODE_SIGNAL_GRAPH:
      drawSignalGraph(rssi);
      break;

    case MODE_NETWORK_SCANNER:
      drawNetworkScanner();
      break;
  }

  // Print to serial
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");

  // Delay between updates
  delay(500);

  // TODO: Add button support to switch modes
  // For now, mode cycles automatically every 10 seconds
  static unsigned long lastModeChange = 0;
  if(millis() - lastModeChange > 10000) {
    currentMode = (DisplayMode)((currentMode + 1) % 3);
    lastModeChange = millis();

    // Clear history when switching to graph mode
    if(currentMode == MODE_SIGNAL_GRAPH) {
      for(int i = 0; i < HISTORY_SIZE; i++) {
        signalHistory[i] = -100;
      }
      historyIndex = 0;
    }
  }
}
