# Simulate idle governor algorithms based on perf sched data

from  src.states import * 
from src.parser import parse_timehist


input_file  = "./.tmp/timehist"
output_file = "./.tmp/timehist.csv"

if parse_timehist(input_file, output_file):
    print(f"Failed to parse {input_file}")
    exit()
