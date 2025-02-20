import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

class Scenario:
    def __init__(self, name, index, thresh):
        self.name = name
        self.index = index
        self.thresh = thresh

def parse_data(data):
    options = ['r', 'w']
    separated_data = []
    for vic in options:
        for att in options:
            separated_data.append(data.loc[(data['vic'] == vic ) & (data['att'] == att)])
    return separated_data

def full_addr_heat_map(data, name):
    # Create a 16x16 grid
    grid = np.zeros((16, 16*16))

    # Populate the grid with latency values
    for _, row in data.iterrows():
        addr = row['addr']
        latency = row['lm_avg']
        row_idx = (addr >> 8) & 0xF  # High nibble for row
        col_idx = addr & 0xFF        # Low byte for column
        grid[row_idx, col_idx] = latency

    # Plot the heatmap
    plt.imshow(grid, cmap='hot', interpolation='nearest', aspect='auto', extent=[0, 256, 16, 0])
    plt.gca().invert_yaxis()
    plt.colorbar(orientation='horizontal', label='Latency')
    # Customize x-axis ticks to show hex addresses
    x_ticks = np.arange(0, 256, 16)  # Show ticks every 16 units
    x_tick_labels = [f'0x{int(x):x}' for x in x_ticks]  # Convert to hex
    plt.xticks(x_ticks, x_tick_labels, rotation=45)

    # Customize y-axis ticks to show hex addresses
    y_ticks = np.arange(0, 16, 1)  # Show ticks every 1 unit
    y_tick_labels = [f'0x{int(y):x}' for y in y_ticks]  # Convert to hex
    plt.yticks(y_ticks, y_tick_labels)
    plt.xlabel('Bits [7:0]')
    plt.ylabel('Bits [11:8]')
    plt.title('Address Latency Heatmap')
    plt.tight_layout()
    plt.savefig(f'data/plots/{name}_fulladdr.png')
    plt.clf()

def partial_addr_heat_map(data, name):
    # Create a 16x16 grid
    grid = np.zeros((16, 16))

    # Populate the grid with latency values
    for _, row in data.iterrows():
        addr = row['addr']
        latency = row['diff_avg']
        row_idx = (addr >> 4) & 0xF  # High nibble for row
        col_idx = addr & 0xF        # Low byte for column
        grid[row_idx, col_idx] += latency
    
    # grid = grid / 16

    # Plot the heatmap
    plt.imshow(grid, cmap='hot', interpolation='nearest', aspect='auto', extent=[0, 16, 16, 0])
    plt.gca().invert_yaxis()
    plt.colorbar(orientation='horizontal', label='Latency')
    # Customize x-axis ticks to show hex addresses
    x_ticks = np.arange(0, 16, 1)  # Show ticks every 16 units
    x_tick_labels = [f'0x{int(x):x}' for x in x_ticks]  # Convert to hex
    plt.xticks(x_ticks, x_tick_labels, rotation=45)

    # Customize y-axis ticks to show hex addresses
    y_ticks = np.arange(0, 16, 1)  # Show ticks every 1 unit
    y_tick_labels = [f'0x{int(y):x}' for y in y_ticks]  # Convert to hex
    plt.yticks(y_ticks, y_tick_labels)
    plt.xlabel('Bits [3:0]')
    plt.ylabel('Bits [7:4]')
    plt.title('Address Latency Heatmap')
    plt.tight_layout()
    plt.savefig(f'data/plots/{name}_partaddr.png')
    plt.clf()

reg_type = "eax"
scenarios = [Scenario('Vr-Ar', 0, 150), Scenario('Vr-Aw', 1, 200), Scenario('Vw-Ar', 2, 160), Scenario('Vw-Aw', 3, 210)]
sep_data = parse_data(pd.read_csv(f"data/csvs/{reg_type}.csv"))
for case in scenarios:
    # full_addr_heat_map(sep_data[case.index], case.name)
    partial_addr_heat_map(sep_data[case.index], case.name)