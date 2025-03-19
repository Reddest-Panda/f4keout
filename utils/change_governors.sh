#!/bin/bash

# Change to max core number
for X in $(seq $1 $2);
do
    cpufreq-set -g $3 -c $X
done

# Validate successful change
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor