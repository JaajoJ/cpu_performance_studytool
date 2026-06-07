# A python port of linux menu https://raw.githubusercontent.com/torvalds/linux/refs/heads/master/drivers/cpuidle/governors/menu.c
import csv
from src.governors.base import governor_cls

def _menu_update():
    pass

def _get_typical_interval_ns(memory_dict) -> int:
    value = -1
    min_thresh = -1
    max_thresh = 18446744073709551615

    while True:
        max = 0
        min = 18446744073709551615
        avg = 0
        variance = 0
        divisor = 0
        for i in range(0, 8):
            value = memory_dict["intervals"][i]
            if value <= min_thresh or value >= max_thresh:
                continue
            divisor += 1
            avg += value
            variance += value * value

            if value > max:
                max = value
            
            if value < min:
                min = value
    
        if not max:
            return 18446744073709551615

        if divisor == 8:
            avg >>= 3
            variance >>= 3
        else:
            avg //= divisor
            variance //= divisor

        avg_sq = avg * avg
        variance -= avg_sq

        U64_MAX = (1 << 64) - 1

        if variance <= U64_MAX // 36:
            if (avg * avg > variance * 36 and divisor * 4 >= 8 * 3) or variance <= 400:
                return avg
        UINT_MAX = (1 << 32) - 1

        if divisor * 4 <= 8 * 3:
            return UINT_MAX

        # Update the thresholds for the next round.
        if avg - min > max - avg:
            min_thresh = min
        else:
            max_thresh = max

def which_bucket(duration_ns: int) -> int:
    bucket = 0
    NSEC_PER_USEC = 1000
    if duration_ns < 10 * NSEC_PER_USEC:     return bucket
    if duration_ns < 100 * NSEC_PER_USEC:    return bucket + 1
    if duration_ns < 1000 * NSEC_PER_USEC:   return bucket + 2
    if duration_ns < 10000 * NSEC_PER_USEC:  return bucket + 3
    if duration_ns < 100000 * NSEC_PER_USEC: return bucket + 4
    return bucket + 5

## SIMULATED KERNEL FUNCTION
def _tick_nohz_tick_stopped():
    # System is tickless thus return is always true
    return True

## SIMULATED KERNEL FUNCTION
def _tick_nohz_get_sleep_length(csv_path: str, cpu: int, timestamp: float) -> int:
    NOHZ_TASKS = (
        'Timer[',       # timerfd/POSIX timer wakeups
        'rcu_',         # RCU grace period timers
        'migration/',   # scheduler tick driven
        'ksoftirqd/',   # softirq timer backlog
        'kworker/',     # kernel worker delayed work
        'Softwar',      # SoftwareVsyncThread (timerfd driven)
    )
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        found_idle = False
        for row in reader:
            if float(row['time']) < timestamp or int(row['cpu']) != cpu:
                continue
            task = row['task'].strip()
            if task == '<idle>':
                found_idle = True
                continue
            if found_idle and any(task.startswith(t) for t in NOHZ_TASKS):
                delta_ms = float(row['time']) - float(row["run_time_ms"]) - timestamp # The timestamps in the data are the end times of each event so we need to compensate using the run_time_ms
                return int(delta_ms * 1_000_000)  # ms -> ns
    return 0

class menu(governor_cls):
    def __init__(self, cpu_states_dict, cpu_idx, csv_path):
        self.cpu_states_dict = cpu_states_dict
        self.cpu_idx = cpu_idx
        self.csv_path = csv_path
        self.data = {
            "needs_update": 0,
            "tick_wakeup": 0,
            "next_timer_ns": 0,
            "bucket": 0,
            "correction_factor": [0] * 6,   # [0, 0, 0, 0, 0, 0]
            "intervals": [0] * 8,          # [0, 0, 0, 0, 0, 0, 0, 0]
            "interval_ptr": 0,
        }


    def menu_select(self, timestamp):
        
        if self.data["needs_update"]:
            _menu_update()
            self.data["needs_update"] = 0

        RESIDENCY_THRESHOLD_NS = 15000
        TICK_NSEC = 4*10**-6
        
        predicted_ns = _get_typical_interval_ns()
        if predicted_ns > predicted_ns or _tick_nohz_tick_stopped():
            delta = _tick_nohz_get_sleep_length(self.csv_path, self.cpu, timestamp)
            if delta < 0:
                delta = 0
                delta_tick = 0
            self.data["next_timer_ns"] = delta
            self.data["bucket"] = which_bucket(self.data["next_timer_ns"])


            NSEC_PER_USEC = 1000

            divisor = 1024 * 8 * NSEC_PER_USEC

            timer_us = ((divisor // 2) + self.data['next_timer_ns'] * self.data['correction_factor'][self.data['bucket']]) // divisor

            predicted_ns = min(timer_us * NSEC_PER_USEC, predicted_ns)
            if (_tick_nohz_tick_stopped() and predicted_ns < TICK_NSEC and self.data["next_timer_ns"] > 2*TICK_NSEC):
                predicted_ns = self.data["next_timer_ns"]
        else:
            self.data["next_timer_ns"] = 9_223_372_036_854_775_807 # ktime_max
            delta_tick = TICK_NSEC / 2
            self.data["bucket"] = 6 - 1
        
        if self.data["next_timer_ns"] < self.cpu_states_dict["C1"]["target_residency"]:
            return 0
        

        




