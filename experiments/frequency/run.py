import os, itertools
import scipy.stats as stats
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from datetime import datetime
from tqdm import tqdm

#! --- Small class for Measurements --- !#

def read_data(filename):
    readings = [[], []]
    with open(f"{filename}", "r") as f:
        for line in f:
            time, freq = line.split()
            readings[0].append(int(time))
            readings[1].append(int(freq))
    return readings

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

def process(readings):
    avg_time = stats.tmean(remove_outliers(readings[0]))
    avg_freq = stats.tmean(remove_outliers(readings[1]))
    return avg_time, avg_freq

def parse_data(data):
    options = ['f', 'r', 'w', 'm']
    separated_data = []
    for vic in options:
        for att in options:
            separated_data.append(data.loc[(data['vic'] == vic )& (data['att'] == att)])
    return separated_data

#! --- Run Tests --- !#
options = ['r', 'w', 'm']
runs = [n for n in range(0x000, 0x1000)] # Trying all offset values
all_cases = [options, runs]

timestamp = datetime.now()
timestamp_data = timestamp.strftime('%d') + '_' + timestamp.strftime('%m') + '_' + timestamp.strftime('%Y') + '_' + timestamp.strftime('%H:%M:%S')

## Setup for data collection
os.makedirs('data/tmp', exist_ok=True)
os.makedirs('data/csvs', exist_ok=True)
os.makedirs('data/plots', exist_ok=True)
os.system("gcc -lpthread -w -O0 -o test contention.c")

## Collect Data
data = pd.DataFrame(columns=["inst", "avg", "freq"])
for att, _ in tqdm(itertools.product(*all_cases), total=(len(options)*len(runs))): 
    os.system(f"sudo ./test {att} > data/tmp/{att}")
    readings = read_data(f'data/tmp/{att}')
    avg_time, avg_freq = process(readings)
    data = pd.concat([data, pd.DataFrame([[att, avg_time, avg_freq]], columns=data.columns)], ignore_index=True) # appending row of data
os.system("rm test")

## Saving / Graphing
data.to_csv(f"data/csvs/{timestamp_data}.csv")