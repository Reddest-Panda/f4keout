import matplotlib.pyplot as plt
import numpy as np

for file_name in ["vr-ar", "vr-aw", "vw-ar", "vw-aw"]:

	f = open("data/" + file_name, "r")
	no_contention = []
	contention = []

	for line in f:
		times = line.split("\t")
		
		if len(times) != 2:
			continue
			
		if max(int(times[0]), int(times[1])) < 400:
			no_contention.append(int(times[0]))
			contention.append(int(times[1]))
	f.close()

	nc_counts, nc_bins = np.histogram(no_contention, bins=100)
	c_counts, c_bins = np.histogram(contention, bins=100)
	
	plt.figure()
	plt.stairs(nc_counts, nc_bins, label="No Aliasing")
	plt.stairs(c_counts, c_bins, label="Aliasing")
	plt.legend()
	plt.show()
