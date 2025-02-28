import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def parse_data(data, scenarios):
    separated_data = []
    for inst in scenarios:
        separated_data.append(data.loc[(data['inst'] == inst)])
    return separated_data

def debug_heat_map(data, name):
    # Create a 16x16 grid
    grid = np.zeros((16, 16*16))

    # Populate the grid with latency values
    for i, row in data.iterrows():
        latency = row['avg']
        row_idx = (i >> 8) & 0xF  # High nibble for row
        col_idx = i & 0xFF        # Low byte for column
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
    plt.savefig(f'data/plots/{name}_debug.png')
    plt.clf()

def freq_vs_cycles_graph(data, name):
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6))

    # Plot Cycles
    ax1.plot(data['avg'].tolist(), marker='.', color='b', label='Cycle Count')
    ax1.set_ylabel('Cycles')

    # Plot Freqs
    ax2.plot(data['freq'].tolist(), marker='.', color='r', label='Freq')
    ax2.set_ylabel('Freq')
    ax2.legend()

    # Save Plots
    plt.tight_layout()
    plt.savefig(f'data/plots/{name}.png')
    plt.clf()

# Thresh is chosen through observation of graph and is therefore imperfect
scenarios = ['r', 'w', 'm']
sep_data = parse_data(pd.read_csv("data/csvs/28_02_2025_12:10:17.csv"), scenarios)
for i, case in enumerate(scenarios):
    debug_heat_map(sep_data[i], case)
    freq_vs_cycles_graph(sep_data[i], case)