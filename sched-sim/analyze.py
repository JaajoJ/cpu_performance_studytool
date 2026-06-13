import csv
from collections import defaultdict
import numpy as np
import matplotlib.pyplot as plt

import matplotlib.widgets as widgets
import numpy as np

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

def estimate_prediction_linear(samplesize = 30, p = 3, max_data=1000) -> dict:
    # Linear prediction algorithm:
    #           Y_hat(t) = SUM(a(i) x Y(t - i))
    #               a: the coefficients
    #               Y: past values

    # Coefficients a
    #           M x = y

    with open(output_file, newline='') as f:
        all_rows = list(csv.DictReader(f))
    
    core_data = {}
    core_data_predictions = {}

    for row in all_rows[:max_data]:
        cpu = int(row["cpu"])
        if cpu not in core_data:
            core_data[cpu] = []
        core_data[cpu].append(row)
    
    sample_size = samplesize
    samples = [0] * sample_size
    p = p

    def autocorr(x, lag):
        x = np.asarray(x)
        #x = x - x.mean() mean 
        n = len(x)
        return np.dot(x[:n-lag], x[lag:])


    for core in core_data.keys():
        #print(core_data[core])
        predictions = [0]
        samples = [0] * sample_size
        actual = []
        #print(core)
        for data_point in core_data[core]:
            if data_point['task'] != '<idle>':
                continue
            #print("data")
            actual.append(float(data_point['run_time_ms']))

            # 1) Offset samples
            samples += [samples.pop(0)]
            samples[-1] = float(data_point['run_time_ms'])
            
            # 2) Calculating normalized autocorrelation coefficients
            r=[]
            for i in range(p + 1):
                r.append(autocorr(samples, i))

            E_value = r[0] # E0 = R0
            
            a_j = [0.0] * p

            # 3) Recursively calculate the coefficients using levinson-durbin method
            for i in range(1, p + 1):
                
                # 3.1) Calculate sums for reflection coefficient also 38b
                a_j_R = sum([ a_j[j - 1] * r[i - j] for j in range(1, i)])

                # 3.2) Calculate reflection coefficient
                k_i = - (r[i] + a_j_R) / E_value

                a_prev = a_j[:]

                a_j[i - 1] = k_i

                # 3.3) Calculate the updated coefficients
                for i2 in range(1, i):
                    a_j[i2-1] = a_prev[i2-1] + k_i * a_prev[i - i2 - 1]

                E_value = (1 - k_i ** 2) * E_value
            
            # 4) prediction using the coefficients
            prediction = -sum(
                a_j[i] * samples[-(i + 1)]
                for i in range(p)
            )
            predictions.append( prediction)
        
        core_data_predictions[core] = {"predictions": predictions[sample_size:-1], "measurements": actual[sample_size:]}

    return core_data_predictions

def plot_predictions(data, figsize_per_core=(12, 3)):
    """
    data = {
        core_id: {
            "predictions":  [values],
            "measurements": [values]
        }
    }
    Interactive single-core viewer with prev/next navigation.
    """
    

    cores = sorted(data.keys())
    state = {"idx": 0}

    fig, axes = plt.subplots(1, 2, figsize=figsize_per_core)
    fig.subplots_adjust(bottom=0.2)

    ax_prev = fig.add_axes([0.3, 0.05, 0.1,  0.07])
    ax_next = fig.add_axes([0.6, 0.05, 0.1,  0.07])
    ax_info = fig.add_axes([0.41, 0.05, 0.18, 0.07])
    ax_info.axis("off")

    btn_prev = widgets.Button(ax_prev, "◀ Prev")
    btn_next = widgets.Button(ax_next, "Next ▶")
    info_text = ax_info.text(0.5, 0.5, "", ha="center", va="center",
                             fontsize=11, transform=ax_info.transAxes)

    def draw(idx):
        core = cores[idx]
        actual = np.asarray(data[core]["measurements"], dtype=float)
        pred   = np.asarray(data[core]["predictions"],  dtype=float)
        n      = min(len(actual), len(pred))
        actual, pred = actual[:n], pred[:n]
        error  = np.abs(actual - pred)
        mae    = np.mean(error)
        rmse   = np.sqrt(np.mean((actual - pred) ** 2))

        for ax in axes:
            ax.cla()

        axes[0].plot(actual, label="Actual")
        axes[0].plot(pred,   label="Predicted", linestyle="--")
        axes[0].set_title(f"Core {core}  |  MAE={mae:.2f}  RMSE={rmse:.2f}")
        axes[0].set_xlabel("Time step")
        axes[0].grid(True, alpha=0.3)
        axes[0].legend()

        axes[1].plot(error, color="tab:red", alpha=0.7)
        axes[1].set_title(f"Core {core}  —  Absolute error")
        axes[1].set_xlabel("Time step")
        axes[1].grid(True, alpha=0.3)

        info_text.set_text(f"{idx + 1} / {len(cores)}")
        fig.canvas.draw_idle()

    def on_prev(_):
        state["idx"] = (state["idx"] - 1) % len(cores)
        draw(state["idx"])

    def on_next(_):
        state["idx"] = (state["idx"] + 1) % len(cores)
        draw(state["idx"])

    btn_prev.on_clicked(on_prev)
    btn_next.on_clicked(on_next)

    draw(0)
    plt.show()

def summarize_predictions(core_data_predictions):

    print(
        f"{'Core':<6} {'N':>6} {'MAE':>12} {'RMSE':>12} "
        f"{'Bias':>12} {'MAPE %':>12} {'Corr':>12}"
    )

    print("-" * 80)

    for core, data in core_data_predictions.items():

        pred = np.asarray(data["predictions"], dtype=float)
        meas = np.asarray(data["measurements"], dtype=float)

        n = min(len(pred), len(meas))

        pred = pred[:n]
        meas = meas[:n]

        error = pred - meas

        mae = np.mean(np.abs(error))
        rmse = np.sqrt(np.mean(error ** 2))
        bias = np.mean(error)

        nonzero = meas != 0
        if np.any(nonzero):
            mape = np.mean(
                np.abs(error[nonzero]) / meas[nonzero]
            ) * 100
        else:
            mape = np.nan

        corr = (
            np.corrcoef(meas, pred)[0, 1]
            if n > 1 else np.nan
        )

        print(
            f"{core:<6} {n:>6} "
            f"{mae:>12.3f} "
            f"{rmse:>12.3f} "
            f"{bias:>12.3f} "
            f"{mape:>12.2f} "
            f"{corr:>12.3f}"
        )

def estimate_prediction_fourier():
    with open(output_file, newline='') as f:
        all_rows = list(csv.DictReader(f))

    x = np.array([float(r['run_time_ms']) for r in all_rows if r['task'] == '<idle>'])
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
    #report_csv()
    #estimate_prediction_fourier()
    core_data_predictions = estimate_prediction_linear(
        samplesize=30,
        p=7,
        max_data=100000
    )
    #print(core_data_predictions[0]["measurements"], core_data_predictions[0]["predictions"])
    summarize_predictions(core_data_predictions)
    plot_predictions(core_data_predictions)
