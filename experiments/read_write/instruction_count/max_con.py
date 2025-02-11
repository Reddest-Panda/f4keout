import numpy as np              
import pandas as pd             
import seaborn as sns           
import matplotlib.pyplot as plt 

# Importing
df = pd.read_csv("data/avg_diffs", sep="\t", header=None, names=["v_ic", "a_ic", "diff"])
df["v_ic"] *= 4     # 4 Instructions per loop
df["a_ic"] *= 4     # 4 Instructions per loop
df = df[df["v_ic"] < 350]

# Finding Max
print(df.loc[[df['diff'].idxmax()]])