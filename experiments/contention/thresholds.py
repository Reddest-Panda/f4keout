import os
import matplotlib.pyplot as plt
from datetime import datetime

workingdir = "/home/skylar_lab/research/implementation/f4keout/" #! Change on system to pwd
basedir = workingdir + "experiments/contention/data"

def get_files(base_dir, file_name):
    thresholds = []
    for dirpath, dirnames, filenames in os.walk(base_dir):
        for filename in filenames:
            if filename == f'{file_name}':
                file_path = os.path.join(dirpath, filename)
                try:
                    with open(file_path, 'r') as file:
                        data = file.readlines()
                except Exception as e:
                    print(f'Error {file_path}: {e}')
                    continue
                thresholds.append(data)

    list_of_list = []
    for i in range(len(thresholds)):
        for idx, value in enumerate(thresholds[i]):
            value = value[1:-1]
            list_of_values = value.split(',')
            list_of_list.append(list_of_values)
    return list_of_list

def plot_data(thresholds, sames, diffs):
    fig, axes = plt.subplots(4, 4, figsize=(15,15))

    for idx, ths in enumerate(thresholds):
        for j in range(len(ths)):
            row = j // 4
            col = j % 4
            ax = axes[row, col]
            #print(f'{row},{col}:{j}:{float(ths[j])}')
            ax.plot(idx,float(ths[j]),'o', color='red')
    for idx, sms in enumerate(sames):
        for j in range(len(sms)):
            row = j // 4
            col = j % 4
            ax = axes[row, col]
            #print(f'{row},{col}:{j}:{float(sms[j])}')
            ax.plot(idx,float(sms[j]),'o',color='orange')
    for idx, dfs in enumerate(diffs):
        for j in range(len(dfs)):
            row = j // 4
            col = j % 4
            ax = axes[row, col]
            #print(f'{row},{col}:{j}:{float(dfs[j])}')
            ax.plot(idx,float(dfs[j]),'o',color='blue')

    row_labels = ["victim flush", "victim read", "victim write", "victim mixed"]
    col_labels = ["attacker flush", "attacker read", "attacker write", "attacker mixed"]

    for ax, col_label in zip(axes[0], col_labels):
        ax.set_title(col_label, pad=30, fontsize=14)
    for ax, row_label in zip(axes[:,0], row_labels):
        ax.set_ylabel(row_label, rotation="vertical", labelpad=40, fontsize=14,
                      va="center")

    fig.text(0.5, 0.04, "run", ha="center", fontsize=14)

    timestamp = datetime.now()
    timestamp_data = timestamp.strftime('%d') + '_' + timestamp.strftime('%m') + '_' +      timestamp.strftime('%Y') + '_' + timestamp.strftime('%H:%M:%S')
    os.makedirs(basedir + "/threshold_plots", exist_ok=True)
    plt.savefig(basedir + f"/threshold_plots/{timestamp_data}.png")

    plt.show()

def main():
    thresholds = get_files(f'{basedir}', 'thresholds.txt')
    sames = get_files(f'{basedir}', 'sames.txt')
    diffs = get_files(f'{basedir}', 'diffs.txt')
    plot_data(thresholds, sames, diffs)
    os.system(f"rm {basedir}/../test")

if __name__ == '__main__':
    main()
