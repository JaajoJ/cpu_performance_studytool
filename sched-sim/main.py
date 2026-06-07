# Simulate idle governor algorithms based on perf sched data

import csv
from  src.states import * 
from src.parser import parse_timehist


input_file  = "./.tmp/timehist"
output_file = "./.tmp/timehist.csv"

if parse_timehist(input_file, output_file):
    print(f"Failed to parse {input_file}")
    exit()

with open(output_file, newline='') as csvfile:
    reader = csv.DictReader(csvfile)

    # ['time', 'cpu', 'task', 'tid_pid', 'wait_time_ms', 'sch_delay_ms', 'run_time_ms', 'task_data_index']

    for row in reader:
        if row["task"] == "<idle>":
            print("IDLE")
        