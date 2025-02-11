import os, time
import scipy.stats as stats
import matplotlib.pyplot as plt
import numpy as np

#! --- Small class for Measurements --- !#
class Monitor:
	def __init__(self, name):
		self.name = name
		self.cycles = []
		self.begin = 0
		self.end = 0

	def read_data(self):
		with open("data/" + self.name, "r") as f:
			for line in f:
				if line.strip() == "|<BEGIN>|":
					self.begin = len(self.cycles)
				elif line.strip() == "|<END>|":
					self.end = len(self.cycles)
				else:
					try:
						self.cycles.append(int(line))
					except Exception:
						continue

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
		# return [x for x in data if lower_bound <= x <= upper_bound]
		return [x for x in data if x <= 1200]

	# Create a figure with a 3x3 grid of subplots
	fig, axes = plt.subplots(3, 3, figsize=(15, 15))

	for idx, measurement in enumerate(measurements):
		row = idx // 3
		col = idx % 3

		ax = axes[row, col]

		# Remove outliers from `diff` and `same` data
		data = remove_outliers(measurement.cycles)
		ax.plot(range(len(data)), data, color='blue', linestyle="None", marker='.')
		ax.axvline(x=measurement.begin, color='red', linestyle='--')
		ax.axvline(x=measurement.end, color='red', linestyle='--')

	# Add overarching row and column labels
	row_labels = ["Victim Flush", "Victim Read", "Victim Write"]
	col_labels = ["Attacker Flush", "Attacker Read", "Attacker Write"]

	for ax, col_label in zip(axes[0], col_labels):
		ax.set_title(col_label, pad=30, fontsize=14)

	for ax, row_label in zip(axes[:, 0], row_labels):
		ax.set_ylabel(row_label, rotation="vertical", labelpad=40, fontsize=14, va='center')

	# Add common x-axis and y-axis labels
	fig.text(0.5, 0.04, "Cycles", ha="center", fontsize=14)

	# Adjust layout
	plt.tight_layout(rect=[0.05, 0.05, 1, 0.95])
	plt.savefig("times.png")

#! --- Run Tests --- !#
## Setup for data collection
os.makedirs("data", exist_ok=True)
os.system("gcc -lpthread -w -O0 -o test ic.c")

## Collect Data
options = ['r', 'w', 'f']
for vic in options:
	for att in options:
		os.system(f"./test {vic} {att} > data/{vic}-{att}")
		time.sleep(0.1)

## Cleanup
os.remove("test")

#! --- Process Data & Print Results --- !#
data = os.listdir("data/")
data.sort()
measurements = []

for file in data:
	test = Monitor(file)
	test.read_data()
	measurements.append(test)

graph(measurements)
os.system("code times.png")