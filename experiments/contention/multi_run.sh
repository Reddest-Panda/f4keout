#!/bin/bash

for i in `seq 1 100`
do
    python3 run.py
done

python3 thresholds.py
