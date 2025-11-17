#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "secrets.h"

// OLED Display configuration for Heltec WiFi Kit 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// NTP Configuration
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = -18000;  // EST = -5 hours, adjust for your timezone
const int DAYLIGHT_OFFSET_SEC = 3600;  // Daylight saving (if applicable)

// Update intervals
const unsigned long PRICE_UPDATE_INTERVAL = 60000;  // 60 seconds
const unsigned long DISPLAY_ROTATION_INTERVAL = 7000; // 7 seconds per asset (more time to read)
const unsigned long TIME_UPDATE_INTERVAL = 1000; // Update time display every second

// Price history for sparkline
#define HISTORY_SIZE 30
struct PriceHistory {
  float prices[HISTORY_SIZE];
  int index;
  bool filled;
};

// Enhanced price data structure
struct Asset {
  String symbol;
  String name;
  float price;
  float change24h;
  float volume24h;
  float marketCap;
  float high24h;
  float low24h;
  bool dataValid;
  unsigned long lastUpdate;
  PriceHistory history;
};

// Assets to track - customize this list!
Asset cryptoAssets[] = {
  {"BTC", "Bitcoin", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}},
  {"ETH", "Ethereum", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}},
  {"SOL", "Solana", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}},
  {"BNB", "Binance Coin", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}}
};

Asset stockAssets[] = {
  {"AAPL", "Apple", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}},
  {"GOOGL", "Google", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}},
  {"TSLA", "Tesla", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}},
  {"MSFT", "Microsoft", 0, 0, 0, 0, 0, 0, false, 0, {{}, 0, false}}
};

const int NUM_CRYPTO = sizeof(cryptoAssets) / sizeof(cryptoAssets[0]);
const int NUM_STOCKS = sizeof(stockAssets) / sizeof(stockAssets[0]);

int currentDisplayIndex = 0;
bool showingCrypto = true;
unsigned long lastDisplayRotation = 0;
unsigned long lastPriceUpdate = 0;
unsigned long lastTimeUpdate = 0;

// API endpoints
const char* CRYPTO_API = "https://api.coingecko.com/api/v3/coins/markets";
const char* STOCK_API = "https://query1.finance.yahoo.com/v8/finance/chart/";

// Forward declarations
void updateCryptoPrices();
void updateStockPrices();
void drawAsset(Asset &asset, bool isCrypto);
void addPriceToHistory(Asset &asset, float price);
void drawSparkline(Asset &asset, int x, int y, int width, int height);
String formatLargeNumber(float num);
String formatVolume(float vol);
String getTrendArrow(float change);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("  Enhanced Crypto & Stock Ticker");
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
  display.println(F("Enhanced Ticker"));
  display.println(F("v2.0"));
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
    display.println(F("Syncing time..."));
    display.display();
    
    // Configure time
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    delay(2000);
    
    display.println(F("Fetching prices..."));
    display.display();
    
    // Initial price fetch
    updateCryptoPrices();
    updateStockPrices();
    
    display.println(F("Ready!"));
    display.display();
    delay(1000);
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

void updateCryptoPrices() {
  if(WiFi.status() != WL_CONNECTED) return;
  
  Serial.println("\n[Crypto] Fetching enhanced data...");
  
  HTTPClient http;
  http.setTimeout(15000);
  
  // Enhanced API call with more data
  String url = String(CRYPTO_API) + 
               "?vs_currency=usd" +
               "&ids=bitcoin,ethereum,solana,binancecoin" +
               "&order=market_cap_desc" +
               "&sparkline=false" +
               "&price_change_percentage=24h";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if(httpCode == 200) {
    String payload = http.getString();
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if(!error) {
      JsonArray coins = doc.as<JsonArray>();
      
      for(JsonObject coin : coins) {
        String id = coin["id"].as<String>();
        int assetIndex = -1;
        
        if(id == "bitcoin") assetIndex = 0;
        else if(id == "ethereum") assetIndex = 1;
        else if(id == "solana") assetIndex = 2;
        else if(id == "binancecoin") assetIndex = 3;
        
        if(assetIndex >= 0) {
          float newPrice = coin["current_price"];
          cryptoAssets[assetIndex].price = newPrice;
          cryptoAssets[assetIndex].change24h = coin["price_change_percentage_24h"];
          cryptoAssets[assetIndex].volume24h = coin["total_volume"];
          cryptoAssets[assetIndex].marketCap = coin["market_cap"];
          cryptoAssets[assetIndex].high24h = coin["high_24h"];
          cryptoAssets[assetIndex].low24h = coin["low_24h"];
          cryptoAssets[assetIndex].dataValid = true;
          cryptoAssets[assetIndex].lastUpdate = millis();
          
          // Add to price history for sparkline
          addPriceToHistory(cryptoAssets[assetIndex], newPrice);
          
          Serial.printf("%s: $%.2f (%s%.2f%%) Vol: %s MCap: %s\n", 
                        cryptoAssets[assetIndex].symbol.c_str(),
                        cryptoAssets[assetIndex].price,
                        cryptoAssets[assetIndex].change24h >= 0 ? "+" : "",
                        cryptoAssets[assetIndex].change24h,
                        formatVolume(cryptoAssets[assetIndex].volume24h).c_str(),
                        formatLargeNumber(cryptoAssets[assetIndex].marketCap).c_str());
        }
      }
      
      Serial.println("[Crypto] Update successful!");
    } else {
      Serial.println("[Crypto] JSON parse error!");
    }
  } else {
    Serial.printf("[Crypto] HTTP error: %d\n", httpCode);
  }
  
  http.end();
}

void updateStockPrices() {
  if(WiFi.status() != WL_CONNECTED) return;
  
  Serial.println("\n[Stock] Fetching enhanced data...");
  
  for(int i = 0; i < NUM_STOCKS; i++) {
    HTTPClient http;
    http.setTimeout(15000);
    
    String url = String(STOCK_API) + stockAssets[i].symbol + "?interval=1d&range=5d";
    
    http.begin(url);
    http.addHeader("User-Agent", "Mozilla/5.0");
    int httpCode = http.GET();
    
    if(httpCode == 200) {
      String payload = http.getString();
      
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if(!error && doc["chart"]["result"][0].is<JsonObject>()) {
        JsonObject result = doc["chart"]["result"][0];
        
        if(result["meta"].is<JsonObject>()) {
          JsonObject meta = result["meta"];
          
          float currentPrice = meta["regularMarketPrice"];
          float previousClose = meta["chartPreviousClose"];
          
          stockAssets[i].price = currentPrice;
          stockAssets[i].change24h = ((currentPrice - previousClose) / previousClose) * 100.0;
          stockAssets[i].high24h = meta["regularMarketDayHigh"] | currentPrice;
          stockAssets[i].low24h = meta["regularMarketDayLow"] | currentPrice;
          stockAssets[i].volume24h = meta["regularMarketVolume"] | 0;
          stockAssets[i].marketCap = meta["marketCap"] | 0;
          stockAssets[i].dataValid = true;
          stockAssets[i].lastUpdate = millis();
          
          // Add to price history
          addPriceToHistory(stockAssets[i], currentPrice);
          
          Serial.printf("%s: $%.2f (%s%.2f%%) Vol: %s MCap: %s\n", 
                        stockAssets[i].symbol.c_str(),
                        stockAssets[i].price,
                        stockAssets[i].change24h >= 0 ? "+" : "",
                        stockAssets[i].change24h,
                        formatVolume(stockAssets[i].volume24h).c_str(),
                        formatLargeNumber(stockAssets[i].marketCap).c_str());
        }
      }
    } else {
      Serial.printf("[Stock] %s HTTP error: %d\n", stockAssets[i].symbol.c_str(), httpCode);
    }
    
    http.end();
    delay(250);
  }
  
  Serial.println("[Stock] Update complete!");
}

void addPriceToHistory(Asset &asset, float price) {
  asset.history.prices[asset.history.index] = price;
  asset.history.index = (asset.history.index + 1) % HISTORY_SIZE;
  if(asset.history.index == 0) asset.history.filled = true;
}

void drawSparkline(Asset &asset, int x, int y, int width, int height) {
  if(!asset.history.filled && asset.history.index < 2) return;
  
  int dataPoints = asset.history.filled ? HISTORY_SIZE : asset.history.index;
  if(dataPoints < 2) return;
  
  // Find min and max for scaling
  float minPrice = asset.history.prices[0];
  float maxPrice = asset.history.prices[0];
  
  for(int i = 0; i < dataPoints; i++) {
    if(asset.history.prices[i] < minPrice) minPrice = asset.history.prices[i];
    if(asset.history.prices[i] > maxPrice) maxPrice = asset.history.prices[i];
  }
  
  float range = maxPrice - minPrice;
  if(range < 0.01) range = asset.price * 0.01; // Avoid division by zero
  
  // Draw sparkline
  for(int i = 1; i < dataPoints && i < width; i++) {
    int idx1 = (asset.history.index + i - 1) % HISTORY_SIZE;
    int idx2 = (asset.history.index + i) % HISTORY_SIZE;
    
    int y1 = y + height - ((asset.history.prices[idx1] - minPrice) / range * height);
    int y2 = y + height - ((asset.history.prices[idx2] - minPrice) / range * height);
    
    display.drawLine(x + i - 1, y1, x + i, y2, SSD1306_WHITE);
  }
}

String formatLargeNumber(float num) {
  if(num >= 1e12) return String(num / 1e12, 1) + "T";
  if(num >= 1e9) return String(num / 1e9, 1) + "B";
  if(num >= 1e6) return String(num / 1e6, 1) + "M";
  if(num >= 1e3) return String(num / 1e3, 1) + "K";
  return String((int)num);
}

String formatVolume(float vol) {
  return "$" + formatLargeNumber(vol);
}

String getTrendArrow(float change) {
  if(change > 5) return "^^";
  if(change > 2) return "^";
  if(change > 0) return "-";
  if(change > -2) return "v";
  if(change > -5) return "vv";
  return "VV";
}

void drawAsset(Asset &asset, bool isCrypto) {
  display.clearDisplay();
  
  // Header with type and time
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(isCrypto ? F("CRYPTO") : F("STOCK"));
  
  // Current time
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    display.setCursor(96, 0);
    display.print(timeStr);
  }
  
  // WiFi indicator
  if(WiFi.status() == WL_CONNECTED) {
    display.setCursor(75, 0);
    int rssi = WiFi.RSSI();
    if(rssi > -60) display.print(F("|||"));
    else if(rssi > -75) display.print(F("|| "));
    else display.print(F("|  "));
  }
  
  display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);
  
  if(asset.dataValid) {
    // Symbol and trend arrow
    display.setTextSize(1);
    display.setCursor(0, 11);
    display.print(asset.symbol);
    display.print(" ");
    display.print(getTrendArrow(asset.change24h));
    
    // Price - large
    display.setTextSize(2);
    display.setCursor(0, 21);
    display.print(F("$"));
    if(asset.price >= 1000) {
      display.printf("%.0f", asset.price);
    } else if(asset.price >= 100) {
      display.printf("%.1f", asset.price);
    } else if(asset.price >= 10) {
      display.printf("%.2f", asset.price);
    } else if(asset.price >= 1) {
      display.printf("%.3f", asset.price);
    } else {
      display.setTextSize(1);
      display.setCursor(6, 25);
      display.printf("%.4f", asset.price);
    }
    
    // 24h change
    display.setTextSize(1);
    display.setCursor(0, 38);
    if(asset.change24h >= 0) {
      display.print(F("+"));
    }
    display.print(asset.change24h, 2);
    display.print(F("%"));
    
    // High/Low
    display.setCursor(40, 38);
    display.print(F("H:"));
    display.print((int)asset.high24h);
    display.setCursor(78, 38);
    display.print(F("L:"));
    display.print((int)asset.low24h);
    
    // Volume and Market Cap
    display.setCursor(0, 47);
    display.print(F("Vol:"));
    display.print(formatVolume(asset.volume24h));
    
    display.setCursor(0, 56);
    display.print(F("MCap:"));
    display.print(formatLargeNumber(asset.marketCap));
    
    // Sparkline on the right
    drawSparkline(asset, 70, 47, 58, 16);
    
  } else {
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.print(F("Loading data..."));
  }
  
  display.display();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Update prices periodically
  if(currentMillis - lastPriceUpdate >= PRICE_UPDATE_INTERVAL) {
    updateCryptoPrices();
    updateStockPrices();
    lastPriceUpdate = currentMillis;
  }
  
  // Rotate display
  if(currentMillis - lastDisplayRotation >= DISPLAY_ROTATION_INTERVAL) {
    if(showingCrypto) {
      drawAsset(cryptoAssets[currentDisplayIndex], true);
      currentDisplayIndex++;
      
      if(currentDisplayIndex >= NUM_CRYPTO) {
        currentDisplayIndex = 0;
        showingCrypto = false;
      }
    } else {
      drawAsset(stockAssets[currentDisplayIndex], false);
      currentDisplayIndex++;
      
      if(currentDisplayIndex >= NUM_STOCKS) {
        currentDisplayIndex = 0;
        showingCrypto = true;
      }
    }
    
    lastDisplayRotation = currentMillis;
  }
  
  delay(100);
}
