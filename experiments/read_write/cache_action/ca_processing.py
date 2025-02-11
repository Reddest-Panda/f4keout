import pandas as pd
import scipy.stats as stats

# Creates Table of Means and SD for each process
for file_name in ["vr-ar", "vr-aw", "vw-ar", "vw-aw"]:

	# Importing
	f = open("data/" + file_name, "r")
	no_contention = []
	contention = []

	for line in f:
		times = line.split("\t")
		
		if len(times) != 2:
			continue
			
		no_contention.append(int(times[0]))
		contention.append(int(times[1]))
	f.close()

	# Pre-Processing (Removes Outliers Based on IQR)
	df_con = pd.DataFrame({'data': contention})
	Q1 = df_con['data'].quantile(.25)
	Q3 = df_con['data'].quantile(.75)
	thresh = 1.5 * (Q3 - Q1) + Q3
	df_con = df_con[df_con['data'] < thresh]

	df_nocon = pd.DataFrame({'data': no_contention})
	Q1 = df_nocon['data'].quantile(.25)
	Q3 = df_nocon['data'].quantile(.75)
	thresh = 1.5 * (Q3 - Q1) + Q3
	df_nocon = df_nocon[df_nocon['data'] < thresh]

	# Printing Data
	print("file: " + file_name)
	print("No Con | Mean: %d\tStdev: %d" % (stats.tmean(df_nocon['data']), stats.tstd(df_nocon['data'])))
	print("Con    | Mean: %d\tStdev: %d" % (stats.tmean(df_con['data']), stats.tstd(df_con['data'])))