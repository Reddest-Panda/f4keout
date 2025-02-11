#!/bin/bash

gcc -lpthread -w -O0 -o ca cache_action.c
./ca r r > data/vr-ar
./ca r w > data/vr-aw
./ca w r > data/vw-ar
./ca w w > data/vw-aw
python3 ca_processing.py