#!/bin/bash

OUTPUT_DIR="./.tmp"
PRERECORDED_DATA="./data/perf.data"

rm -rf ./.tmp/*
mkdir -p $OUTPUT_DIR


# Recording 

if [ -f "$PRERECORDED_DATA" ]; then
    echo "Prerecorded data exists. Will not record seperately..."
    cp $PRERECORDED_DATA $OUTPUT_DIR
else
    echo "Recording perf data"
    if ! command -v perf &> /dev/null; then
        echo "Error: perf not found" >&2
        exit 1
    fi
    sudo perf sched record -o $OUTPUT_DIR/perf.data -- sleep 60
    
    if command -v powertop &> /dev/null; then
        sudo powertop --html=$OUTPUT_DIR/report.html
    fi

    sudo chown $USER:$USER $OUTPUT_DIR/*
fi


# Parsing

perf sched timehist -i $OUTPUT_DIR/perf.data > $OUTPUT_DIR/timehist
perf sched map -i $OUTPUT_DIR/perf.data > $OUTPUT_DIR/map
python3 parse-data.py