import numpy as np              
import pandas as pd             
import seaborn as sns           
import matplotlib.pyplot as plt 

# Importing
df = pd.read_csv("data/avg_diffs", sep="\t", header=None, names=["v_ic", "a_ic", "diff"])
df["v_ic"] *= 4     # 4 Instructions per loop
df["a_ic"] *= 4     # 4 Instructions per loop
data = df.pivot(index="v_ic", columns="a_ic", values="diff")

# Plotting
fig, ax = plt.subplots(figsize=(6.5, 5.5), facecolor="#FDF7E2")

heatmap = sns.heatmap(data, cmap='magma', ax=ax)
for _, spine in heatmap.spines.items():
    spine.set_visible(True)
    spine.set_color('black')
    spine.set_linewidth(1.5)

cbar = ax.collections[0].colorbar
cbar.outline.set_linewidth(1.5)
cbar.outline.set_edgecolor('black')

# Invert so origin is 0,0
ax.invert_yaxis()

# Adjusting Ticks
x_ticks = np.linspace(0, len(data.columns) - 1, 15, dtype=int)
y_ticks = np.linspace(0, len(data.index) - 1, 15, dtype=int)
ax.set_xticks(x_ticks)
ax.set_yticks(y_ticks)
ax.set_xticklabels([data.index[i] for i in y_ticks],fontsize=10)
ax.set_yticklabels([data.index[i] for i in y_ticks], fontsize=10)

# Labels
heatmap.set_xlabel("Attacker Instruction Count", fontsize=18)
heatmap.set_ylabel("Victim Instruction Count", fontsize=18)
plt.show()