#!/bin/bash

ERRMSG=$'Incorrect Usage: proper usage is ./run.sh [option]. Default is option compare.\noptions: None, compare, thread, normal'

make
# ./thread > data/times_thread
./normal > data/times_normal

# rm thread
rm normal

if [[ "$1" == "compare" ||  "$1" == "thread"  ||  "$1" == "normal" || "$1" == "" ]]
then
    python3 graph.py "$1"
    code data/plot.png
else
    echo "$ERRMSG"
fi