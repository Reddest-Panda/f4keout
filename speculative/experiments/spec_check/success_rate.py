import subprocess
import sys

def getData(filename):
	hits = [0] * 256
	f = open("data/" + filename, "r")
	its = float(f.readline())
	for line in f:
		index = int(line)
		hits[index] += 1
	f.close()
	return [n / its for n in hits]

def runTest(test):
	# Run and Process
	with open(f"data/times_{test}", "w") as outfile:
		subprocess.run(["taskset", "-c", "0", f"./{test}"], stdout=outfile, check=True)
	data = getData(f"times_{test}")

	print(f"Success Rate for {test}: {data[73]:0.2%}")

def argParser():
	errmsg = "Incorrect Usage: proper usage is success_rate.py [option]. Default is option compare.\noptions: None, compare, thread, normal"

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

if __name__ == "__main__":
	# Argument Check
	compare, thread, normal = argParser()

	# Make
	subprocess.run(["make"], check=True)

	# Run Tests
	if (compare | thread):
		runTest("thread")
	if (compare | normal):
		runTest("normal")

	# Clean
	subprocess.run(["rm", "thread"], check=True)
	subprocess.run(["rm", "normal"], check=True)