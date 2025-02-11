import os, time
import scipy.stats as stats
import matplotlib.pyplot as plt
import numpy as np

#! --- Small class for Measurements --- !#
class Measurement:
	def __init__(self, name):
		self.name = name

		self.diff = []
		self.diff_mean = 0
		self.diff_std = 0

		self.upper_match = []
		self.um_mean = 0
		self.um_std = 0
		
		self.lower_match = []
		self.lm_mean = 0
		self.lm_std = 0
	
	def __str__(self):
		table = f"""
Victim: {(self.name).split('-')[0]} / Attacker: {(self.name).split('-')[1]}   
+------------------+----------------+
| Different Offset | {self.diff_mean:>6.2f} ± {self.diff_std:<5.2f} |
+------------------+----------------+
| Same Offset      | {self.same_mean:>6.2f} ± {self.same_std:<5.2f} |
+------------------+----------------+
"""
		return table

	def read_data(self):
		data_readings = [self.diff, self.upper_match, self.lower_match]
		i = 0
		with open("data/" + self.name, "r") as f:
			for line in f:
				if line.strip() == "|<END>|":
					i += 1
					continue
				data_readings[i].append(int(line))

	def process(self):
		self.diff_mean = stats.tmean(self.diff)
		self.diff_std = stats.tstd(self.diff)

		self.same_mean = stats.tmean(self.same)
		self.same_std = stats.tstd(self.same)

def graph(measurements):
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

	# Create a figure with a 3x3 grid of subplots
	fig, axes = plt.subplots(4, 4, figsize=(15, 15))

	for idx, measurement in enumerate(measurements):
		row = idx // 4
		col = idx % 4

		ax = axes[row, col]

		# Remove outliers from `diff`, `lower_match`, and `upper_match` data
		diff_filtered = remove_outliers(measurement.diff)
		lower_match_filtered = remove_outliers(measurement.lower_match)
		upper_match_filtered = remove_outliers(measurement.upper_match)

		# Compute histogram data for filtered `diff`, `lower_match`, and `upper_match`
		min_value = min(
			min(diff_filtered, default=0), 
			min(lower_match_filtered, default=0), 
			min(upper_match_filtered, default=0)
		)
		max_value = max(
			max(diff_filtered, default=1), 
			max(lower_match_filtered, default=1), 
			max(upper_match_filtered, default=1)
		)
		bins = np.arange(min_value, max_value + 2, 2)  # Bins grouped by 2 units

		diff_hist, _ = np.histogram(diff_filtered, bins=bins)
		lower_match_hist, _ = np.histogram(lower_match_filtered, bins=bins)
		upper_match_hist, _ = np.histogram(upper_match_filtered, bins=bins)

		# Plot line graph for filtered `diff`, `lower_match`, and `upper_match` data
		bin_centers = (bins[:-1] + bins[1:]) / 2
		ax.plot(bin_centers, diff_hist, label='Diff', color='blue', linestyle='-')
		ax.plot(bin_centers, lower_match_hist, label='Lower Match', color='orange', linestyle='-')
		ax.plot(bin_centers, upper_match_hist, label='Same', color='green', linestyle='-')

		ax.legend()

	# Add overarching row and column labels
	row_labels = ["Victim Flush", "Victim Read", "Victim Write", "Victim Mixed"]
	col_labels = ["Attacker Flush", "Attacker Read", "Attacker Write", "Attacker Mixed"]

	for ax, col_label in zip(axes[0], col_labels):
		ax.set_title(col_label, pad=30, fontsize=14)

	for ax, row_label in zip(axes[:, 0], row_labels):
		ax.set_ylabel(row_label, rotation="vertical", labelpad=40, fontsize=14, va='center')

	# Add common x-axis and y-axis labels
	fig.text(0.5, 0.04, "Cycles", ha="center", fontsize=14)

	# Adjust layout
	plt.tight_layout(rect=[0.05, 0.05, 1, 0.95])
	plt.savefig("times.png")

def compare_victim_read_write(measurements):
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

	# Remove outliers from `diff`, `lower_match`, and `upper_match` data
	diff_combined = measurements[0].diff + measurements[1].diff
	diff_filtered = remove_outliers(diff_combined)
	victim_read_filtered = remove_outliers(measurements[0].lower_match)
	victim_write_filtered = remove_outliers(measurements[1].lower_match)

	# Compute histogram data for filtered `diff`, `lower_match`, and `upper_match`
	min_value = min(
		min(diff_filtered, default=0), 
		min(victim_write_filtered, default=0), 
		min(victim_read_filtered, default=0)
	)
	max_value = max(
		max(diff_filtered, default=1), 
		max(victim_write_filtered, default=1), 
		max(victim_read_filtered, default=1)
	)
	bins = np.arange(min_value, max_value + 2, 2)  # Bins grouped by 2 units

	diff_hist, _ = np.histogram(diff_filtered, bins=bins)
	victim_write_hist, _ = np.histogram(victim_write_filtered, bins=bins)
	victim_read_hist, _ = np.histogram(victim_read_filtered, bins=bins)

	# Plot line graph for filtered `diff`, `lower_match`, and `upper_match` data
	fig, ax = plt.subplots()
	bin_centers = (bins[:-1] + bins[1:]) / 2
	ax.plot(bin_centers, diff_hist, label='Diff', color='blue', linestyle='-')
	ax.plot(bin_centers, victim_read_hist, label='Victim Reads', color='green', linestyle='-')
	ax.plot(bin_centers, victim_write_hist, label='Victim Writes', color='orange', linestyle='-')

	ax.legend()

	plt.savefig("compare.png")

#! --- Run Tests --- !#
ITS = 100
## Setup for data collection
os.makedirs("data", exist_ok=True)
os.system("gcc -lpthread -w -O0 -o test contention.c")
measurements = []

## Collect Data
options = ['f', 'r', 'w', 'm']
for vic in options:
	for att in options:
		curr_test = Measurement(f"{vic}-{att}")
		for _ in range(ITS):
			os.system(f"./test {vic} {att} > data/{vic}-{att}")
			curr_test.read_data()
			time.sleep(0.01) # Buffer
		measurements.append(curr_test)

## Cleanup
os.remove("test")

#! --- Process Data & Print Results --- !#
graph(measurements)
os.system("code times.png")

#! --- Testing --- !#
att_flush = [measurements[5], measurements[8]]
print([data.name for data in att_flush])
compare_victim_read_write(att_flush)
os.system("code compare.png")