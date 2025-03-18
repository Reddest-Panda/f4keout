#!/bin/bash

for X in $(seq 0 23);
do
    cpufreq-set -g performance -c $X
done
