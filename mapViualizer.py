# map_visualizer.py
import matplotlib
# CRITICAL: 'Agg' backend renders images in the background without needing a UI window!
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# Grid Configuration
GRID_WIDTH  = 51
GRID_HEIGHT = 26

# Bitmasks & Types
CONFIDENCE_MASK = 0b00011111
TYPE_MASK       = 0b00000011
FLAG_MASK       = 0b00000001

WALL  = 1 #blue
EMPTY = 2 #green
NOGO  = 3 #red

def unpackPoint(value):
    confidence = value & CONFIDENCE_MASK
    cell_type  = (value >> 5) & TYPE_MASK
    return confidence, cell_type

def render_and_save():
    # 1. Read the raw binary data saved by main.py
    try:
        with open("sweep_data.bin", "rb") as f:
            renderSweep = bytearray(f.read()) #opens the sweep file then saves it to the renderSweep array

    except FileNotFoundError:
        print("Error: sweep_data.bin not found.")
        return

    if len(renderSweep) != (GRID_WIDTH * GRID_HEIGHT):
        print("Error: Invalid map data size.")
        return

    # 2. Convert data into an RGBA image array
    rgba_matrix = np.zeros((GRID_HEIGHT, GRID_WIDTH, 4)) #makes an array GRID_HEIGHT by GRID_WIDTH with

    for y in range(GRID_HEIGHT): #loop through the whole array to draw it to matrix. This is super slow and should be replaced with a better method eventually
        for x in range(GRID_WIDTH):
            idx = (y * GRID_WIDTH) + x
            val = renderSweep[idx]

            confidence, cell_type = unpackPoint(val)
            brightness = 0.1 + 0.9 * (confidence / 31)

            if cell_type == WALL:
                rgba_matrix[y, x] = [0.1, 0.1, 1.0, brightness*3] #right now wall confidence is only 0-5, multiply by 3 to make it brigher and give more range
            elif cell_type == EMPTY:
                rgba_matrix[y, x] = [0.1, 1.0, 0.1, brightness]
            elif cell_type == NOGO:
                rgba_matrix[y, x] = [1.0, 0.1, 0.1, brightness]
            else:
                rgba_matrix[y, x] = [0.08, 0.08, 0.08, 1.0]

    # 3. Build the Matplotlib figure
    fig, ax = plt.subplots(figsize=(10, 6))
    fig.patch.set_facecolor('#121212')
    ax.set_facecolor('#121212')
    ax.spines['bottom'].set_color('#444444')
    ax.spines['top'].set_color('#444444')
    ax.spines['left'].set_color('#444444')
    ax.spines['right'].set_color('#444444')
    ax.xaxis.label.set_color('white')
    ax.yaxis.label.set_color('white')
    ax.tick_params(colors='white')

    extent = [-GRID_WIDTH // 2, GRID_WIDTH // 2, 0, GRID_HEIGHT]
    ax.imshow(rgba_matrix, origin='lower', extent=extent)

    ax.set_title("Sweep Map", color='white', fontsize=14, pad=12)
    ax.set_xlabel("Grid X (cells)")
    ax.set_ylabel("Grid Y (cells)")

    # 4. Save to disk, overwriting the previous image
    plt.savefig("map.png", dpi=100, bbox_inches='tight', facecolor=fig.get_facecolor())

    # 5. VERY IMPORTANT: Close the figure to free up memory
    plt.close(fig)

if __name__ == '__main__':
    render_and_save()
