import numpy as np              
import pandas as pd             
import seaborn as sns           
import matplotlib.pyplot as plt 

# Importing
df = pd.read_csv("data/data", sep="\t", header=None, names=["ic", "min", "avg", "max"])
df["ic"] = (df["ic"].str.replace(':', '')).astype(int)
df = df.drop(["min", "max"], axis="columns")
df = df[df["ic"] <= 420]

#Plotting
fig, ax = plt.subplots(figsize=(9.5, 3.15), facecolor="#FDF7E2")
ax.plot(df["ic"], df["avg"], marker='o', mfc="#314762", mec="#314762", ms=2, c="#A5FD01")

#Cosmetics
ax.set_xlabel("Instruction Count", fontsize=18)
ax.set_ylabel("Avg. Cycles", fontsize=18)
ax.grid()

plt.show()