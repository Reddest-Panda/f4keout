#!/bin/bash

gcc -lpthread -w -O0 -o ic instruct_count.c

for i in {5..1000..5}
do
    for j in {5..1000..5}
    do
        ./ic $j $i > data/diffs
        python3 ic_preprocessing.py >> data/avg_diffs
    done
done

#python3 data_processing.py