#!/bin/sh

make
./test 4095 > data/same_offset
./test 273 > data/diff_offset
python3 graphing.py
code data/plot.png