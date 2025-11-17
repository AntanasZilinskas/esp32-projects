#!/usr/bin/env python3
"""
WiFi Speed Mapper - Log WiFi speeds at different locations in your house
"""

import speedtest
import json
import time
from datetime import datetime
import os
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
from matplotlib.backends.backend_pdf import PdfPages

DATA_FILE = "data/wifi_measurements.json"
FLOOR_PLAN_1 = "data/google_images.png"  # Floor 1 plan
FLOOR_PLAN_2 = "data/google_images.png"  # Floor 2 plan (you can use same or different image)

def load_measurements():
    """Load existing measurements from file"""
    if os.path.exists(DATA_FILE):
        with open(DATA_FILE, 'r') as f:
            return json.load(f)
    return {"floor1": [], "floor2": []}

def save_measurements(data):
    """Save measurements to file"""
    with open(DATA_FILE, 'w') as f:
        json.dump(data, f, indent=2)

def run_speed_test():
    """Run a speed test and return results in Mbps"""
    print("\n🔄 Running speed test... (this may take 30-60 seconds)")
    try:
        st = speedtest.Speedtest()
        st.get_best_server()

        print("   Testing download speed...")
        download_speed = st.download() / 1_000_000  # Convert to Mbps

        print("   Testing upload speed...")
        upload_speed = st.upload() / 1_000_000  # Convert to Mbps

        ping = st.results.ping

        return {
            "download_mbps": round(download_speed, 2),
            "upload_mbps": round(upload_speed, 2),
            "ping_ms": round(ping, 2)
        }
    except Exception as e:
        print(f"❌ Error running speed test: {e}")
        return None

def get_location_from_image(floor_plan_path, floor_num):
    """Get X, Y coordinates by clicking on floor plan image"""
    if not os.path.exists(floor_plan_path):
        print(f"⚠️  Floor plan image not found: {floor_plan_path}")
        print("   Falling back to manual coordinate entry")
        return get_location_manual()

    print(f"\n📍 Click on the floor plan to mark your location (Floor {floor_num})")
    print("   A window will open - click where you are, then close the window")

    try:
        # Load and display the image
        img = mpimg.imread(floor_plan_path)
        fig, ax = plt.subplots(figsize=(12, 10))
        ax.imshow(img)
        ax.set_title(f'Click your location on Floor {floor_num}\n(Close window after clicking)',
                    fontsize=14, fontweight='bold')
        ax.set_xlabel('Click anywhere on the image')

        # Store clicked coordinates
        coords = {'x': None, 'y': None}

        def onclick(event):
            if event.xdata is not None and event.ydata is not None:
                coords['x'] = round(event.xdata, 2)
                coords['y'] = round(event.ydata, 2)
                # Mark the clicked point
                ax.plot(event.xdata, event.ydata, 'rx', markersize=15, markeredgewidth=3)
                ax.set_title(f'Location marked! Close this window to continue.\nCoordinates: ({coords["x"]:.1f}, {coords["y"]:.1f})',
                           fontsize=14, fontweight='bold', color='green')
                plt.draw()

        # Connect the click event
        cid = fig.canvas.mpl_connect('button_press_event', onclick)

        plt.show()

        if coords['x'] is not None and coords['y'] is not None:
            print(f"✅ Location marked at: ({coords['x']:.1f}, {coords['y']:.1f})")
            return coords
        else:
            print("❌ No location clicked. Using (0, 0)")
            return {"x": 0, "y": 0}

    except Exception as e:
        print(f"❌ Error loading image: {e}")
        print("   Falling back to manual coordinate entry")
        return get_location_manual()

def get_location_manual():
    """Get X, Y coordinates from user manually (fallback)"""
    print("\n📍 Enter your location on the floor:")
    print("   (0,0) = corner of room, increase numbers as you move away")
    print("   Try to use consistent spacing (e.g., 1 unit = 1 meter)")

    try:
        x = float(input("   X coordinate (horizontal): "))
        y = float(input("   Y coordinate (vertical): "))
        return {"x": x, "y": y}
    except ValueError:
        print("❌ Invalid input. Using (0, 0)")
        return {"x": 0, "y": 0}

def main():
    """Main loop for collecting WiFi measurements"""
    print("=" * 60)
    print("🏠 WiFi Speed Heatmap Mapper")
    print("=" * 60)

    measurements = load_measurements()

    print(f"\nCurrent measurements:")
    print(f"  Floor 1: {len(measurements['floor1'])} points")
    print(f"  Floor 2: {len(measurements['floor2'])} points")

    while True:
        print("\n" + "-" * 60)
        print("Options:")
        print("  1 - Take measurement on Floor 1")
        print("  2 - Take measurement on Floor 2")
        print("  3 - View all measurements")
        print("  4 - Clear all measurements")
        print("  q - Quit and generate heatmaps")

        choice = input("\nChoice: ").strip().lower()

        if choice == 'q':
            break
        elif choice == '1' or choice == '2':
            floor_key = f"floor{choice}"
            floor_num = choice

            # Use floor plan image for location
            floor_plan = FLOOR_PLAN_1 if choice == '1' else FLOOR_PLAN_2
            location = get_location_from_image(floor_plan, floor_num)
            speed_results = run_speed_test()

            if speed_results:
                measurement = {
                    "timestamp": datetime.now().isoformat(),
                    "location": location,
                    "speeds": speed_results
                }

                measurements[floor_key].append(measurement)
                save_measurements(measurements)

                print("\n✅ Measurement saved!")
                print(f"   Download: {speed_results['download_mbps']} Mbps")
                print(f"   Upload: {speed_results['upload_mbps']} Mbps")
                print(f"   Ping: {speed_results['ping_ms']} ms")
                print(f"   Location: ({location['x']}, {location['y']})")
                print(f"\n   Total measurements on Floor {choice}: {len(measurements[floor_key])}")

        elif choice == '3':
            print("\n📊 All Measurements:")
            for floor in ['floor1', 'floor2']:
                floor_num = floor[-1]
                print(f"\n  Floor {floor_num} ({len(measurements[floor])} points):")
                for i, m in enumerate(measurements[floor], 1):
                    loc = m['location']
                    speed = m['speeds']
                    print(f"    {i}. ({loc['x']}, {loc['y']}) - {speed['download_mbps']} Mbps down")

        elif choice == '4':
            confirm = input("⚠️  Clear ALL measurements? (yes/no): ")
            if confirm.lower() == 'yes':
                measurements = {"floor1": [], "floor2": []}
                save_measurements(measurements)
                print("✅ All measurements cleared!")

        else:
            print("❌ Invalid choice")

    print("\n" + "=" * 60)
    print("💾 Measurements saved to:", DATA_FILE)
    print("🎨 Run 'python3 generate_heatmap.py' to create heatmap images")
    print("=" * 60)

if __name__ == "__main__":
    main()
