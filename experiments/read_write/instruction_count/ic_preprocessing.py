import pandas as pd
import scipy.stats as stats

# Importing
f = open("data/diffs", "r")
diffs = []
ics = (f.readline()).replace("\n", "")
for line in f:
	times = line.split("\t")
	
	if len(times) != 1:
		continue
		
	diffs.append(int(times[0]))
f.close()

# Pre-Processing (Removes Outliers Based on IQR)
df = pd.DataFrame({'data': diffs})

Q1 = df['data'].quantile(.25)
Q3 = df['data'].quantile(.75)

upper_thresh = Q3 + 1.5 * (Q3 - Q1)
lower_thresh = Q1 - 1.5 * (Q3 - Q1)

df = df[df['data'] < upper_thresh]
df = df[df['data'] > lower_thresh]

# Printing Data
print("%s\t%d" % (ics, stats.tmean(df['data'])))