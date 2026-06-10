import csv
from collections import defaultdict
import numpy as np
import matplotlib.pyplot as plt

output_file = "./.tmp/timehist.csv"


def report_csv():
    with open(output_file, newline='') as f:
        all_rows = list(csv.DictReader(f))

    if not all_rows:
        print("No data found.")
        return

    # ── Collect stats ────────────────────────────────────────────────────────

    total_rows = len(all_rows)

    times       = [float(r['time'])          for r in all_rows]
    cpus        = [int(r['cpu'])             for r in all_rows]
    wait_times  = [float(r['wait_time_ms'])  for r in all_rows]
    sch_delays  = [float(r['sch_delay_ms'])  for r in all_rows]
    run_times   = [float(r['run_time_ms'])   for r in all_rows]

    time_span   = max(times) - min(times)
    cpu_set     = sorted(set(cpus))

    # Per-task aggregates
    task_stats  = defaultdict(lambda: {'count': 0, 'run_ms': 0.0,
                                       'wait_ms': 0.0, 'delay_ms': 0.0})
    for r in all_rows:
        t = r['task']
        task_stats[t]['count']    += 1
        task_stats[t]['run_ms']   += float(r['run_time_ms'])
        task_stats[t]['wait_ms']  += float(r['wait_time_ms'])
        task_stats[t]['delay_ms'] += float(r['sch_delay_ms'])

    # Per-CPU aggregates
    cpu_stats = defaultdict(lambda: {'count': 0, 'run_ms': 0.0})
    for r in all_rows:
        c = int(r['cpu'])
        cpu_stats[c]['count']  += 1
        cpu_stats[c]['run_ms'] += float(r['run_time_ms'])

    # Top offenders by scheduling delay
    top_delay = sorted(
        task_stats.items(),
        key=lambda x: x[1]['delay_ms'],
        reverse=True
    )[:5]

    # Top CPU consumers
    top_cpu = sorted(
        task_stats.items(),
        key=lambda x: x[1]['run_ms'],
        reverse=True
    )[:5]

    def avg(lst):
        return sum(lst) / len(lst) if lst else 0.0

    # ── Print report ─────────────────────────────────────────────────────────

    sep = "─" * 60

    print(sep)
    print("  SCHEDULER TRACE REPORT")
    print(sep)

    print(f"\n{'OVERVIEW':}")
    print(f"  Total events      : {total_rows:,}")
    print(f"  Time range        : {min(times):.6f} → {max(times):.6f}  ({time_span*1000:.3f} ms span)")
    print(f"  CPUs observed     : {cpu_set}  ({len(cpu_set)} cores)")
    print(f"  Unique tasks      : {len(task_stats)}")

    print(f"\n{'TIMING SUMMARY (ms)':}")
    print(f"  {'Metric':<25} {'Min':>8} {'Avg':>8} {'Max':>8} {'Total':>10}")
    print(f"  {'─'*25} {'─'*8} {'─'*8} {'─'*8} {'─'*10}")
    for label, values in [
        ("run_time_ms",   run_times),
        ("wait_time_ms",  wait_times),
        ("sch_delay_ms",  sch_delays),
    ]:
        print(f"  {label:<25} {min(values):>8.3f} {avg(values):>8.3f} "
              f"{max(values):>8.3f} {sum(values):>10.3f}")

    print(f"\n{'TOP 5 TASKS BY CPU TIME (run_time_ms)':}")
    print(f"  {'Task':<30} {'Events':>7} {'Total ms':>10} {'Avg ms':>9}")
    print(f"  {'─'*30} {'─'*7} {'─'*10} {'─'*9}")
    for task, s in top_cpu:
        print(f"  {task:<30} {s['count']:>7} {s['run_ms']:>10.3f} "
              f"{s['run_ms']/s['count']:>9.3f}")

    print(f"\n{'TOP 5 TASKS BY SCHEDULING DELAY (sch_delay_ms)':}")
    print(f"  {'Task':<30} {'Events':>7} {'Total ms':>10} {'Avg ms':>9}")
    print(f"  {'─'*30} {'─'*7} {'─'*10} {'─'*9}")
    for task, s in top_delay:
        print(f"  {task:<30} {s['count']:>7} {s['delay_ms']:>10.3f} "
              f"{s['delay_ms']/s['count']:>9.3f}")

    print(f"\n{'PER-CPU BREAKDOWN':}")
    print(f"  {'CPU':>4} {'Events':>8} {'Total run ms':>14} {'Avg run ms':>11}")
    print(f"  {'─'*4} {'─'*8} {'─'*14} {'─'*11}")
    for c in sorted(cpu_stats):
        s = cpu_stats[c]
        print(f"  {c:>4} {s['count']:>8} {s['run_ms']:>14.3f} "
              f"{s['run_ms']/s['count']:>11.3f}")

    print(f"\n{'ALL TASKS (alphabetical)':}")
    print(f"  {'Task':<30} {'Count':>7} {'Run ms':>9} {'Wait ms':>9} {'Delay ms':>10}")
    print(f"  {'─'*30} {'─'*7} {'─'*9} {'─'*9} {'─'*10}")
    for task in sorted(task_stats):
        s = task_stats[task]
        print(f"  {task:<30} {s['count']:>7} {s['run_ms']:>9.3f} "
              f"{s['wait_ms']:>9.3f} {s['delay_ms']:>10.3f}")

    print(f"\n{sep}\n")


def estimate_prediction_linear():
    pass

def estimate_prediction_fourier():
    with open(output_file, newline='') as f:
        all_rows = list(csv.DictReader(f))

    x = np.array([float(r['run_time_ms']) for r in all_rows])
    x = x / x.mean()

    X   = np.fft.rfft(x)
    mag = np.abs(X)
    freqs = np.fft.rfftfreq(len(x))

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))

    ax1.plot(x)
    ax1.set(xlabel="sample", ylabel="normalized run_time_ms")

    ax2.bar(freqs, mag, width=freqs[1])
    ax2.set(xlabel="frequency", ylabel="|X[k]|")

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    report_csv()
    estimate_prediction_fourier()
    estimate_prediction_linear()