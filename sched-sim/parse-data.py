import re
import csv
import sys
 
# Usage: python parse_perf.py input.txt [output.csv]
# Defaults: output to perf_sched.csv if not specified
 
input_file  = "./.tmp/timehist"
output_file = "./.tmp/timehist.csv"
 
pattern = re.compile(
    r'^\s*([\d.]+)'   # time
    r'\s+\[(\d+)\]'   # cpu
    r'\s+(\S+)'       # task_name (e.g. perf[62447] or <idle>)
    r'\s+([\d.]+)'    # wait_time_ms
    r'\s+([\d.]+)'    # sch_delay_ms
    r'\s+([\d.]+)'    # run_time_ms
)
 
tid_pattern = re.compile(r'^(.*?)\[(\d+)\]$')
 
rows = []
process_data_index = {}
data_index = 0
with open(input_file) as f:
    for line in f:
        m = pattern.match(line)
        if not m:
            continue
        time, cpu, task_name, wait, sch, run = m.groups()
 
        tid_m = tid_pattern.match(task_name)
        if tid_m:
            task   = tid_m.group(1)
            tid_pid = tid_m.group(2)
        else:
            task    = task_name  # e.g. <idle>
            tid_pid = ""

        if task not in process_data_index:
            process_data_index[task] = data_index
            data_index += 1

        rows.append({
            "time":         float(time),
            "cpu":          int(cpu),
            "task":         task,
            "tid_pid":      tid_pid,
            "wait_time_ms": float(wait),
            "sch_delay_ms": float(sch),
            "run_time_ms":  float(run),
            "task_data_index": process_data_index[task]
        })
 
if not rows:
    print("No data rows found — check the input file format.")
    sys.exit(1)
 
with open(output_file, "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)
 
print(f"Wrote {len(rows)} rows to {output_file}")
