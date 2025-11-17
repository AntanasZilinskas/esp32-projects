# WiFi Speed Heatmap Mapper

Map WiFi speeds throughout your house and generate beautiful heatmap visualizations for each floor.

## Features

- Run speed tests at different locations
- Track measurements for multiple floors
- Generate heatmap images showing download/upload speeds and ping
- Save all data for future analysis

## Setup

1. **Install dependencies:**
   ```bash
   cd wifi-heatmap
   pip3 install -r requirements.txt
   ```

2. **Make scripts executable (optional):**
   ```bash
   chmod +x wifi_mapper.py generate_heatmap.py
   ```

## Usage

### Step 1: Collect Measurements

Run the mapper and walk around your house:

```bash
python3 wifi_mapper.py
```

**Instructions:**
1. Choose which floor you're on (1 or 2)
2. Enter your X and Y coordinates
   - Use consistent spacing (e.g., 1 unit = 1 meter)
   - Start from a corner as (0,0)
   - Example: living room corner = (0,0), 3 meters right = (3,0), 2 meters forward = (3,2)
3. Wait for the speed test to complete (~30-60 seconds)
4. Move to next location and repeat

**Tips:**
- Take at least 5-8 measurements per floor for good heatmaps
- Space measurements evenly across the floor
- Include problem areas where WiFi seems slow
- Include areas near the router where WiFi is fast

### Step 2: Generate Heatmaps

After collecting measurements:

```bash
python3 generate_heatmap.py
```

This will create heatmap images in the `output/` folder:
- `floor_1_download.png` - Download speeds for Floor 1
- `floor_1_upload.png` - Upload speeds for Floor 1
- `floor_1_ping.png` - Ping times for Floor 1
- `floor_2_download.png` - Download speeds for Floor 2
- (etc.)

## Understanding the Heatmaps

- **Green areas** = Fast speeds / good connection
- **Yellow areas** = Medium speeds
- **Red areas** = Slow speeds / weak connection
- **Black X marks** = Your measurement locations
- **Numbers** = Actual speed at that location

## File Structure

```
wifi-heatmap/
├── wifi_mapper.py          # Main data collection script
├── generate_heatmap.py     # Heatmap generation script
├── requirements.txt        # Python dependencies
├── data/
│   └── wifi_measurements.json  # Your collected data
└── output/
    └── *.png              # Generated heatmap images
```

## Troubleshooting

**Speed test fails:**
- Check your internet connection
- Try again (sometimes servers are busy)
- Make sure you're connected to your WiFi network

**Heatmap looks weird:**
- You need at least 3 measurements per floor
- Try adding more measurements in different areas
- Make sure coordinates are spread out (not all in same spot)

**Can't install dependencies:**
```bash
# Try using pip instead of pip3
pip install -r requirements.txt

# Or install individually
pip3 install speedtest-cli numpy matplotlib scipy
```

## Example Workflow

1. Start in corner of Floor 1, mark as (0, 0)
2. Walk 2 meters right, mark as (2, 0), run test
3. Walk 2 meters forward, mark as (2, 2), run test
4. Continue creating a grid across the floor
5. Go to Floor 2, repeat the process
6. Generate heatmaps to see results
7. Use results to optimize router placement!

## Notes

- Data is saved in `data/wifi_measurements.json`
- You can re-run the heatmap generator anytime
- Clear measurements from the menu if you want to start over
- Each speed test takes 30-60 seconds - be patient!
