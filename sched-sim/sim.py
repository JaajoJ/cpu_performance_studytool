import csv
from src.states import intel_skylake_c_states
from src.states import intel_sapphire_rapids_c_states
from src.parser import parse_timehist
from src.governors.menu import menu
from concurrent.futures import ProcessPoolExecutor
import time

input_file  = "./.tmp/timehist"
output_file = "./.tmp/timehist.csv"

if parse_timehist(input_file, output_file):
        print(f"Failed to parse {input_file}")
        exit()

def simulate_governor(cpu_idx, c_states_dict):
    time.sleep(cpu_idx + 1)
    CPU = cpu_idx
    NSEC_PER_MSEC = 1_000_000
    TICK_TASKS = ('rcu_', 'migration/', 'ksoftirqd/', 'kworker/')

    # Summary values
    c_states_entered = [0] * len(c_states_dict.keys())
    c_states_residency_ms = [0] * len(c_states_dict.keys())
    poll_residency_ms = 0

    all_rows = []
    with open(output_file, newline='') as f:
        reader = csv.DictReader(f)
        all_rows = [row for row in reader if row['cpu'] == str(cpu_idx)]
    all_rows = all_rows[:2000]
    gov = menu(cpu_states_dict=c_states_dict, cpu_idx=CPU, rows=all_rows)

    in_idle       = False
    last_state_idx = 0

    for row_idx, row in enumerate(all_rows):
        if int(row['cpu']) != CPU:
            continue

        task   = row['task'].strip()
        time_ms = float(row['time'])
        run_ms  = float(row['run_time_ms'])

        if task == '<idle>':
            entry_ms = time_ms - run_ms

            if in_idle:
                gov.menu_reflect(
                    index             = last_state_idx,
                    last_residency_ns = int(run_ms * NSEC_PER_MSEC),
                    tick_wakeup       = False,
                )

            last_state_idx = gov.menu_select(entry_ms, row_idx)
            #print(f"selected {last_state_idx}")
            c_states_entered[last_state_idx] += 1
            c_states_residency_ms[last_state_idx] += run_ms
            in_idle = True

        else:
            if in_idle:
                tick_wu = any(task.startswith(t) for t in TICK_TASKS)
                gov.menu_reflect(
                    index             = last_state_idx,
                    last_residency_ns = int(run_ms * NSEC_PER_MSEC),
                    tick_wakeup       = tick_wu,
                )
                in_idle = False
                poll_residency_ms += run_ms

    print(" - " * 10)
    print(f"{CPU} - CPU summary")
    print(" - " * 10)
    print(f"        Polling ms: {poll_residency_ms}")
    print(f"        Relative res.:  {100 * poll_residency_ms / (sum(c_states_residency_ms) + poll_residency_ms):.3f}%")
    for key in c_states_dict:
        print("    " + key + ":")
        print(f"        Entered:        {c_states_entered[c_states_dict[key]['idx']]}")
        print(f"        Residency (ms): {c_states_residency_ms[c_states_dict[key]['idx']]:.3f}")
        print(f"        Relative res.:  {100 * c_states_residency_ms[c_states_dict[key]['idx']] / (sum(c_states_residency_ms) + poll_residency_ms):.3f}%")

#simulate_governor(0, intel_skylake_c_states)


def main():
    #cpu_indices = [0, 1, 2, 3, 4, 5, 6, 7]
    cpu_indices=[i for i in range(0,64)]

    with ProcessPoolExecutor(max_workers=6) as executor:
        futures = [
            executor.submit(simulate_governor, cpu_idx, intel_skylake_c_states)
            for cpu_idx in cpu_indices
        ]

        results = [f.result() for f in futures]

if __name__ == "__main__":
    main()
