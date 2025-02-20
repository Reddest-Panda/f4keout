import os, itertools
import scipy.stats as stats
import numpy as np
import pandas as pd
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
    return diff_avg, lm_avg

#! --- Run Tests --- !#
reg_type = 'eax'
options = ['r', 'w']
addrs = [n for n in range(0x000, 0x100)] # Trying all offset values
all_cases = [options, options, addrs]

## Setup for data collection
os.makedirs('data/tmp', exist_ok=True)
os.makedirs('data/csvs', exist_ok=True)
os.makedirs('data/plots', exist_ok=True)
os.system(f"gcc regs/{reg_type}.c -lpthread -w -O0 -o test")

## Collect Data
data = pd.DataFrame(columns=["vic", "att", "addr", "diff_avg", "lm_avg"])
vic = 'r'
att = 'r'
for vic, att, addr in tqdm(itertools.product(*all_cases), total=(len(options)*len(options)*len(addrs))): 
    cmd = (f"./test {vic} {att} {addr} > data/tmp/{vic}-{att}")
    os.system(f'{cmd}')
    readings = read_data(f'data/tmp/{vic}-{att}')
    diff_avg, lm_avg = process(readings[0], readings[1])
    data = pd.concat([data, pd.DataFrame([[vic, att, addr, diff_avg, lm_avg]], columns=data.columns)], ignore_index=True) # appending row of data
os.system("rm test")
data.to_csv(f"data/csvs/{reg_type}.csv")