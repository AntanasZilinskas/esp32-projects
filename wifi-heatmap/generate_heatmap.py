#!/usr/bin/env python3
"""
Generate heatmap images from WiFi speed measurements
"""

import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
from scipy.interpolate import griddata
import os

DATA_FILE = "data/wifi_measurements.json"
OUTPUT_DIR = "output"
FLOOR_PLAN_1 = "data/google_images.png"
FLOOR_PLAN_2 = "data/google_images.png"

def load_measurements():
    """Load measurements from file"""
    if not os.path.exists(DATA_FILE):
        print(f"❌ No data file found at {DATA_FILE}")
        print("   Run wifi_mapper.py first to collect measurements")
        return None

    with open(DATA_FILE, 'r') as f:
        return json.load(f)

def create_heatmap(floor_data, floor_name, metric='download_mbps', floor_plan_path=None):
    """Create a heatmap for a specific floor"""
    if len(floor_data) < 3:
        print(f"⚠️  Need at least 3 measurements for {floor_name}. Currently have {len(floor_data)}.")
        return False

    # Extract coordinates and speeds
    x_coords = [m['location']['x'] for m in floor_data]
    y_coords = [m['location']['y'] for m in floor_data]
    speeds = [m['speeds'][metric] for m in floor_data]

    # Create grid for interpolation
    x_min, x_max = min(x_coords), max(x_coords)
    y_min, y_max = min(y_coords), max(y_coords)

    # Add some padding
    padding = 50  # pixels for floor plan
    x_min = max(0, x_min - padding)
    x_max += padding
    y_min = max(0, y_min - padding)
    y_max += padding

    # Create the plot
    fig, ax = plt.subplots(figsize=(14, 12))

    # If floor plan exists, load and display it as background
    if floor_plan_path and os.path.exists(floor_plan_path):
        try:
            img = mpimg.imread(floor_plan_path)
            img_height, img_width = img.shape[:2]

            # Display the floor plan
            ax.imshow(img, extent=[0, img_width, img_height, 0], alpha=0.6, zorder=1)

            # Set limits based on image size
            x_max = max(x_max, img_width)
            y_max = max(y_max, img_height)
        except Exception as e:
            print(f"   Warning: Could not load floor plan: {e}")

    # Create fine grid for heatmap overlay
    grid_x, grid_y = np.meshgrid(
        np.linspace(x_min, x_max, 300),
        np.linspace(y_min, y_max, 300)
    )

    # Interpolate data
    grid_speeds = griddata(
        (x_coords, y_coords),
        speeds,
        (grid_x, grid_y),
        method='cubic',
        fill_value=np.mean(speeds)
    )

    # Create heatmap overlay with transparency
    contour = ax.contourf(grid_x, grid_y, grid_speeds, levels=20, cmap='RdYlGn', alpha=0.4, zorder=2)
    plt.colorbar(contour, label=f'{metric.replace("_", " ").title()}', ax=ax)

    # Plot measurement points
    ax.scatter(x_coords, y_coords, c='blue', s=150,
              marker='o', linewidths=2, zorder=5, edgecolors='white')

    # Add labels for each point
    for i, (x, y, speed) in enumerate(zip(x_coords, y_coords, speeds)):
        ax.annotate(f'{speed:.1f}',
                   (x, y),
                   xytext=(5, 5),
                   textcoords='offset points',
                   fontsize=10,
                   fontweight='bold',
                   bbox=dict(boxstyle='round,pad=0.4', facecolor='white', alpha=0.9, edgecolor='blue'),
                   zorder=6)

    ax.set_xlabel('X Coordinate (pixels)', fontsize=12)
    ax.set_ylabel('Y Coordinate (pixels)', fontsize=12)
    ax.set_title(f'{floor_name} - WiFi Speed Heatmap\n{metric.replace("_", " ").title()}',
                fontsize=14, fontweight='bold')

    # Invert y-axis to match image coordinates (top-left origin)
    ax.invert_yaxis()

    # Save the figure
    metric_name = metric.replace('_mbps', '').replace('_ms', '')
    filename = f"{OUTPUT_DIR}/{floor_name.lower().replace(' ', '_')}_{metric_name}.png"
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"✅ Saved: {filename}")

    plt.close()
    return True

def generate_all_heatmaps():
    """Generate heatmaps for all floors and metrics"""
    print("=" * 60)
    print("🎨 WiFi Heatmap Generator")
    print("=" * 60)

    measurements = load_measurements()
    if not measurements:
        return

    # Ensure output directory exists
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Generate heatmaps for each floor
    for floor_key in ['floor1', 'floor2']:
        floor_num = floor_key[-1]
        floor_name = f"Floor {floor_num}"
        floor_data = measurements[floor_key]

        print(f"\n{floor_name}: {len(floor_data)} measurements")

        if len(floor_data) < 3:
            print(f"  ⚠️  Skipping - need at least 3 measurements (have {len(floor_data)})")
            continue

        # Select appropriate floor plan
        floor_plan = FLOOR_PLAN_1 if floor_num == '1' else FLOOR_PLAN_2

        # Generate download speed heatmap
        print(f"  Generating download speed heatmap...")
        create_heatmap(floor_data, floor_name, metric='download_mbps', floor_plan_path=floor_plan)

        # Generate upload speed heatmap
        print(f"  Generating upload speed heatmap...")
        create_heatmap(floor_data, floor_name, metric='upload_mbps', floor_plan_path=floor_plan)

        # Generate ping heatmap
        print(f"  Generating ping heatmap...")
        create_heatmap(floor_data, floor_name, metric='ping_ms', floor_plan_path=floor_plan)

    print("\n" + "=" * 60)
    print("✅ All heatmaps generated!")
    print(f"📁 Check the '{OUTPUT_DIR}/' folder for images")
    print("=" * 60)

def show_summary():
    """Show summary of collected data"""
    measurements = load_measurements()
    if not measurements:
        return

    print("\n📊 Data Summary:")
    for floor_key in ['floor1', 'floor2']:
        floor_num = floor_key[-1]
        floor_data = measurements[floor_key]

        if not floor_data:
            continue

        speeds = [m['speeds']['download_mbps'] for m in floor_data]
        print(f"\n  Floor {floor_num}:")
        print(f"    Measurements: {len(floor_data)}")
        print(f"    Avg Download: {np.mean(speeds):.2f} Mbps")
        print(f"    Min Download: {min(speeds):.2f} Mbps")
        print(f"    Max Download: {max(speeds):.2f} Mbps")

if __name__ == "__main__":
    show_summary()
    generate_all_heatmaps()
