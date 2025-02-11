import matplotlib.pyplot as plt
import pandas as pd
import sys

def argParser():
	errmsg = "Incorrect Usage: proper usage is graph.py [option]. Default is option compare.\noptions: None, compare, thread, normal"

	if len(sys.argv) > 2:
		print(errmsg)
		sys.exit()
	elif len(sys.argv) < 2:
		return True, False, False
	else:
		match sys.argv[1].lower().strip():
			case "compare":
				return True, False, False
			case "thread":
				return False, True, False
			case "normal":
				return False, False, True
			case "":
				return True, False, False
			case _:
				print(errmsg)
				sys.exit()

def getData(filename):
	data = [0] * 256
	f = open("data/" + filename, "r")
	for line in f:
		split = line.split()
		if len(split) == 2:
			data[int(split[0])] = int(split[1])
	f.close()
	return data


if __name__ == "__main__":
	# Argument Check
	compare, thread, normal = argParser()


	# Create the plot
	if (compare | thread):
		data_threaded = getData("times_thread")
		plt.plot(range(len(data_threaded)), data_threaded, marker='.', linestyle='-', color='blue', alpha=.5)
	if (compare | normal):
		data_normal = getData("times_normal")
		plt.plot(range(len(data_normal)), data_normal, marker='.', linestyle='-', color='orange', alpha=.5)

	# Cache hit should be on 73
	plt.axvline(x=73, color='r', linestyle=':', alpha=0.3)

	# Add labels and title
	plt.xlabel('Char')
	plt.ylabel('Cycles')
	plt.title('Flush Reload Tests')

	# Show the plot
	plt.savefig("data/plot.png")