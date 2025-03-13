import os, itertools
import scipy.stats as stats
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from datetime import datetime
from tqdm import tqdm

#! --- Small class for Measurements --- !#

def read_data(filename):
    data_readings = [[], []]
    i = 0
    with open(f"{filename}", "r") as f:
        for line in f:
            if line.strip() == "|<END>|":
                i += 1
                continue
            data_readings[i].append(int(line))
    return data_readings

def remove_outliers(data, threshold=1.5):
    """Removes outliers from the data using the IQR method."""
    if len(data) == 0:
        return data
    q1 = np.percentile(data, 25)
    q3 = np.percentile(data, 75)
    iqr = q3 - q1
    lower_bound = q1 - threshold * iqr
    upper_bound = q3 + threshold * iqr
    return [x for x in data if lower_bound <= x <= upper_bound]

def process(diff, lm):
    diff_avg = stats.tmean(remove_outliers(diff))
    lm_avg = stats.tmean(remove_outliers(lm))
    thresh_avg = (diff_avg + lm_avg) / 2
    return diff_avg, lm_avg, thresh_avg

def parse_data(data):
    options = ['r', 'w']
    separated_data = []
    for vic in options:
        for att in options:
            separated_data.append(data.loc[(data['vic'] == vic ) & (data['att'] == att)])
    return separated_data

def graph_all(data):
    # Create a figure with a 4x4 grid of subplots
    fig, axes = plt.subplots(2, 2, figsize=(15, 15))
    separated_data = parse_data(data)
    for idx, scenario_data in enumerate(separated_data):
        ax = axes[idx // 2, idx % 2]
        x_axis = [n for n in range(len(scenario_data['diff_avg']))]

        ax.scatter(x_axis, scenario_data['diff_avg'].values, label='Diff', color='blue', linestyle='dotted')
        ax.scatter(x_axis, scenario_data['lm_avg'], label='4k Aliasing', color='orange', linestyle='dotted')
        ax.scatter(x_axis, scenario_data['thresh_avg'], label='Thresh', color='red', linestyle='dotted')

    # Add overarching row and column labels
    row_labels = ["Victim Read", "Victim Write"]
    col_labels = ["Attacker Read", "Attacker Write"]

    for ax, col_label in zip(axes[0], col_labels):
        ax.set_title(col_label, pad=30, fontsize=14)

    for ax, row_label in zip(axes[:, 0], row_labels):
        ax.set_ylabel(row_label, rotation="vertical", labelpad=40, fontsize=14, va='center')

    # Adjust layout
    plt.tight_layout(rect=[0.05, 0.05, 1, 0.95])
    plt.savefig("data/plots/contention.png")


#! --- Run Tests --- !#
ITS = 100
options = ['r', 'w']
all_cases = [options, options]

timestamp = datetime.now()
timestamp_data = timestamp.strftime('%d') + '_' + timestamp.strftime('%m') + '_' + timestamp.strftime('%Y') + '_' + timestamp.strftime('%H:%M:%S')

## Setup for data collection
os.makedirs('data/tmp', exist_ok=True)
os.makedirs('data/plots', exist_ok=True)
os.makedirs('data/csvs', exist_ok=True)
os.system("gcc -lpthread -w -O0 -o test xmm.c")

## Collect Data
data = pd.DataFrame(columns=["vic", "att", "diff_avg", "lm_avg", "thresh_avg"])
for _ in tqdm(range(ITS)):
    for vic, att in itertools.product(*all_cases): # Full sweep loop
        os.system(f"./test {vic} {att} > data/tmp/{vic}-{att}")
        readings = read_data(f'data/tmp/{vic}-{att}')
        diff_avg, lm_avg, thresh_avg = process(readings[0], readings[1])
        data = pd.concat([data, pd.DataFrame([[vic, att, diff_avg, lm_avg, thresh_avg]], columns=data.columns)], ignore_index=True) # appending row of data

os.system("rm test")
# ## Saving / Graphing
data.to_csv(f"data/csvs/{timestamp_data}.csv")
graph_all(data)