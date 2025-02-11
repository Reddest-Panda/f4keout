import matplotlib.pyplot as plt
import pandas as pd

# Importing
def get_data(filename):
	data = []
	f = open("data/" + filename, "r")
	for line in f:
		if len(line.split()) == 1:
			data.append(int(line))
	f.close()
	return data

same = get_data("same_offset")
diff = get_data("diff_offset")

# Pre-Processing (Removes Outliers Based on IQR)
same_df = pd.DataFrame({'data': same})
Q1 = same_df['data'].quantile(.25)
Q3 = same_df['data'].quantile(.75)
thresh = 1.5 * (Q3 - Q1) + Q3
same_df = same_df[same_df['data'] < thresh]

diff_df = pd.DataFrame({'data': diff})
Q1 = diff_df['data'].quantile(.25)
Q3 = diff_df['data'].quantile(.75)
thresh = 1.5 * (Q3 - Q1) + Q3
diff_df = diff_df[diff_df['data'] < thresh]

# Plot histograms of both DataFrames
plt.hist(same_df['data'], bins=50, density=True, histtype='step', label='same offset', color='blue')
plt.hist(diff_df['data'], bins=50, density=True, histtype='step', label='diff offset', color='orange')

# Add mean points
plt.plot([same_df['data'].mean()], [0], marker='o', markersize=10, color='blue')
plt.plot([diff_df['data'].mean()], [0], marker='o', markersize=10, color='orange')

# Add labels and legend
plt.xlabel('Attacker Cycles')
plt.ylabel('Frequency')
plt.title('Comparing Contention Experienced')
plt.legend()

# Show the plot
plt.savefig("data/plot.png")