# Crypto & Stock Price Ticker

A real-time cryptocurrency and stock price display for ESP32 with OLED screen. Automatically rotates through your chosen assets in a carousel display.

## Features

- **Live Crypto Prices**: Bitcoin, Ethereum, Solana, Binance Coin (customizable)
- **Live Stock Prices**: AAPL, GOOGL, TSLA, MSFT (customizable)
- **Carousel Display**: Automatically rotates through all assets every 5 seconds
- **24-Hour Change**: Shows percentage change with visual indicator
- **WiFi Connectivity**: Updates prices every 60 seconds
- **Signal Strength**: WiFi indicator on display
- **Plug & Play**: Flash once, plug into any outlet with WiFi

## Display Information

Each asset shows:
- Asset symbol (BTC, AAPL, etc.)
- Full name
- Current price in USD
- 24-hour percentage change
- Visual change indicator (bar graph)
- WiFi signal strength

## Hardware Requirements

- **ESP32 Development Board** (Heltec WiFi Kit 32 recommended)
- **OLED Display** (SSD1306, 128x64 pixels)
  - Built-in on Heltec WiFi Kit 32
- **Power supply** (USB or outlet adapter)

## Quick Start

### 1. Edit WiFi Credentials

Open [src/main.cpp](src/main.cpp) and update lines 19-20:

```cpp
const char* WIFI_SSID = "YourWiFiName";       // Change this
const char* WIFI_PASSWORD = "YourPassword";    // Change this
```

### 2. Customize Assets (Optional)

Edit the asset arrays in [src/main.cpp](src/main.cpp) around lines 31-44 to track different cryptocurrencies or stocks:

```cpp
Asset cryptoAssets[] = {
  {"BTC", "Bitcoin", 0, 0, false, 0},
  {"ETH", "Ethereum", 0, 0, false, 0},
  // Add more crypto here
};

Asset stockAssets[] = {
  {"AAPL", "Apple", 0, 0, false, 0},
  {"GOOGL", "Google", 0, 0, false, 0},
  // Add more stocks here
};
```

### 3. Build and Upload

```bash
cd crypto-stock-ticker
python3 -m platformio run --target upload
```

### 4. Plug It In!

Once flashed, just plug your ESP32 into any power source (USB adapter, power bank, etc.) within WiFi range and it will:
1. Connect to WiFi automatically
2. Fetch latest prices
3. Start displaying the carousel
4. Update prices every 60 seconds

## API Information

### Cryptocurrency Prices
- **Provider**: CoinGecko API
- **Rate Limit**: Free tier allows ~10-30 requests/minute
- **No API Key Required**: Works out of the box
- **Supported Coins**: See [CoinGecko Coin List](https://api.coingecko.com/api/v3/coins/list)

### Stock Prices
- **Provider**: Yahoo Finance API
- **Rate Limit**: Generous free tier
- **No API Key Required**: Works out of the box
- **Supported Stocks**: All major exchanges (NYSE, NASDAQ, etc.)

## Customization Options

### Change Display Rotation Speed

In [src/main.cpp](src/main.cpp) line 23:

```cpp
const unsigned long DISPLAY_ROTATION_INTERVAL = 5000; // milliseconds
```

### Change Price Update Frequency

In [src/main.cpp](src/main.cpp) line 22:

```cpp
const unsigned long PRICE_UPDATE_INTERVAL = 60000; // milliseconds
```

Note: Don't set this too low or you may hit API rate limits.

### Add More Assets

To add more cryptocurrencies, find the coin ID on CoinGecko:
1. Visit https://api.coingecko.com/api/v3/coins/list
2. Find your coin's `id` (e.g., "cardano", "polkadot")
3. Add to the array and update the API URL in `updateCryptoPrices()`

For stocks, just use the ticker symbol (e.g., "NVDA", "AMZN").

## Display Modes

The ticker automatically cycles through:
1. **Crypto Assets** (all configured cryptocurrencies)
2. **Stock Assets** (all configured stocks)
3. Repeats continuously

## Troubleshooting

### WiFi Not Connecting
- Check SSID and password in the code
- Ensure ESP32 is within WiFi range
- Check serial monitor for connection status
- Some 5GHz-only networks won't work (ESP32 needs 2.4GHz)

### No Price Data
- Check internet connection
- Verify APIs are accessible (CoinGecko may be blocked in some regions)
- Check serial monitor for HTTP error codes
- Wait 60 seconds for first update

### Display Issues
- Verify I2C pins match your board (Heltec: SDA=4, SCL=15, RST=16)
- Try I2C address 0x3D if 0x3C doesn't work
- Check OLED is firmly connected

### API Rate Limiting
- Increase `PRICE_UPDATE_INTERVAL` to reduce requests
- Remove assets you don't need to track
- Consider using fewer symbols

## Serial Monitor

To see debug output and price updates:

```bash
python3 -m platformio device monitor
```

You'll see:
- WiFi connection status
- Price fetch attempts
- Current prices for all assets
- HTTP response codes
- Update timestamps

## Power Consumption

- **Normal Operation**: ~150-200mA @ 5V (0.75-1W)
- **Sleep Mode**: Not implemented (could be added for battery operation)
- **Power Source**: Any USB power adapter (phone charger works great)

## Future Enhancements

Potential additions:
- [ ] Button to pause/resume carousel
- [ ] Manual mode switching (all crypto, all stocks, mixed)
- [ ] Price alerts (visual indicator when threshold crossed)
- [ ] Historical price graphs
- [ ] Support for multiple currencies (EUR, GBP, etc.)
- [ ] Battery operation with deep sleep
- [ ] Web interface for configuration
- [ ] NTP time synchronization with clock display
- [ ] Market cap and volume data

## Privacy & Security

- **No data sent**: Only receives price data from APIs
- **No personal info**: Doesn't transmit any personal information
- **WiFi credentials**: Stored locally on ESP32 only
- **No account required**: Works without any sign-ups

## License

MIT License - Feel free to modify and share!

## Credits

- **CoinGecko API**: https://www.coingecko.com/en/api
- **Yahoo Finance API**: https://finance.yahoo.com
- **Adafruit Libraries**: Display drivers
