import os

def uncomment_inst(i):
    with open('tmp.c', 'r') as f:
        lines = f.readlines()

    lines[58+i] = lines[58+i].replace('// ','')
    lines[82+i] = lines[82+i].replace('// ','')

    with open('tmp.c', 'w') as f:
        f.writelines(lines)


# os.system('cp contention.c tmp.c')
uncomment_inst(3)