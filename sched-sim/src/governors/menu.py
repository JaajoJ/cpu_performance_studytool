# A python port of linux menu governor
# https://raw.githubusercontent.com/torvalds/linux/refs/heads/master/drivers/cpuidle/governors/menu.c
# This file was developed with the assistance of Claude (Anthropic) via claude.ai to translate .c code to python abstractions

import csv
from src.governors.base import governor_cls

# --- Constants (mirrors C #defines) ------------------------------------------

BUCKETS         = 6
INTERVAL_SHIFT  = 3
INTERVALS       = 1 << INTERVAL_SHIFT   # 8
RESOLUTION      = 1024
DECAY           = 8
NSEC_PER_USEC   = 1000
UINT_MAX        = (1 << 32) - 1
U64_MAX         = (1 << 64) - 1
KTIME_MAX       = (1 << 63) - 1         # s64 max  (ktime_max)

# Corresponds to RESIDENCY_THRESHOLD_NS in the kernel (15 µs expressed in ns)
RESIDENCY_THRESHOLD_NS = 15_000

# Nominal tick period (250 Hz → 4 ms expressed in ns)
TICK_NSEC = 4_000_000

# "Safe timer range" – if the next timer fires within this window, the tick is
# already a sufficient safety net (kernel uses SAFE_TIMER_RANGE_NS = TICK_NSEC).
SAFE_TIMER_RANGE_NS = TICK_NSEC

MAX_INTERESTING = 50_000 * NSEC_PER_USEC   # 50 ms in ns

# ------------------------------------------------------------------------------


def get_typical_interval_ns(memory_dict: dict) -> int:
    """
    Try to detect a repeating wakeup pattern by examining the last INTERVALS
    residency samples.  Returns the estimated interval in *nanoseconds*
    (samples are stored in microseconds, so the return value is scaled up),
    or UINT_MAX * NSEC_PER_USEC when no reliable pattern is found.

    Mirrors get_typical_interval() in menu.c.
    """
    # Sentinel values; -1 means "no lower threshold yet"
    min_thresh: int = -1
    max_thresh: int = UINT_MAX

    while True:
        max_val  = 0
        min_val  = UINT_MAX
        avg      = 0
        variance = 0
        divisor  = 0

        for i in range(INTERVALS):
            value = memory_dict["intervals"][i]

            # Discard samples outside the current [min_thresh, max_thresh] window.
            if value <= min_thresh or value >= max_thresh:
                continue

            divisor  += 1
            avg      += value
            variance += value * value

            if value > max_val:
                max_val = value
            if value < min_val:
                min_val = value

        if not max_val:
            # No usable samples at all → signal "unknown, use UINT_MAX".
            return UINT_MAX * NSEC_PER_USEC

        if divisor == INTERVALS:
            avg      >>= INTERVAL_SHIFT
            variance >>= INTERVAL_SHIFT
        else:
            avg      //= divisor
            variance //= divisor

        avg_sq    = avg * avg
        variance -= avg_sq          # variance = E[x²] - E[x]²

        # Accept the average when the distribution is tight enough.
        if variance <= U64_MAX // 36:
            if (avg_sq > variance * 36 and divisor * 4 >= INTERVALS * 3) or variance <= 400:
                # avg is in µs; convert to ns before returning.
                return avg * NSEC_PER_USEC

        # Too many outliers and sample set is already at 3/4 → give up.
        if divisor * 4 <= INTERVALS * 3:
            return UINT_MAX * NSEC_PER_USEC

        # Narrow the window by dropping the more extreme outlier side.
        if avg - min_val > max_val - avg:
            min_thresh = min_val
        else:
            max_thresh = max_val
        # loop (goto again)


def which_bucket(duration_ns: int) -> int:
    """Return the correction-factor bucket index for a given duration."""
    if duration_ns < 10   * NSEC_PER_USEC: return 0
    if duration_ns < 100  * NSEC_PER_USEC: return 1
    if duration_ns < 1000 * NSEC_PER_USEC: return 2
    if duration_ns < 10_000  * NSEC_PER_USEC: return 3
    if duration_ns < 100_000 * NSEC_PER_USEC: return 4
    return 5


# --- Simulated kernel helpers -------------------------------------------------

def _tick_nohz_tick_stopped() -> bool:
    """The simulated system is always tickless."""
    return True


def _tick_nohz_get_sleep_length(rows: list, start_idx: int, cpu: int, timestamp: float) -> int:
    """
    Return the time (in nanoseconds) until the next timer-driven wakeup task
    appears after *timestamp* on *cpu*, scanning forward from *start_idx* in
    the pre-loaded row list.

    start_idx should be the index of the <idle> row that marks the entry into
    the idle period, so the scan begins exactly there and never re-reads rows
    that are already in the past.

    The CSV rows represent scheduling events; each row has at minimum:
        time        – end-time of the event (float, milliseconds)
        run_time_ms – duration of the event (float, milliseconds)
        cpu         – CPU index (int)
        task        – task name (str)

    Because timestamps in the data are *end* times we compensate by
    subtracting run_time_ms to get the start time of the event.
    """
    NOHZ_TASKS = (
        'Timer[',       # timerfd / POSIX timer wakeups
        'rcu_',         # RCU grace-period timers
        'migration/',   # scheduler-tick driven
        'ksoftirqd/',   # softirq timer backlog
        'kworker/',     # kernel worker delayed work
        'Softwar',      # SoftwareVsyncThread (timerfd driven)
    )
    found_idle = False
    for row in rows[start_idx:]:
        if int(row['cpu']) != cpu:
            continue
        task = row['task'].strip()
        if task == '<idle>':
            found_idle = True
            continue
        #if found_idle and any(task.startswith(t) for t in NOHZ_TASKS):
        if found_idle and task is not "<idle>": #any(task.startswith(t) for t in NOHZ_TASKS):
            start_ms = float(row['time']) - float(row['run_time_ms'])
            delta_ms = start_ms - timestamp
            return int(delta_ms * 1_000_000)  # ms → ns
    return 0


# --- Governor class -----------------------------------------------------------

class menu(governor_cls):
    """
    Python simulation of the Linux 'menu' cpuidle governor.

    Parameters
    ----------
    cpu_states_dict : dict
        Mapping of state name → state attributes dict.  Each state dict must
        contain at least:
            "target_residency_ns" – minimum idle duration to break even (int, ns)
              OR "target_residency" in microseconds (converted automatically)
            "exit_latency_ns"     – wakeup latency of the state (int, ns)
              OR "exit_latency" in microseconds (converted automatically)
        The polling state (C0) is detected automatically as the entry with
        both exit_latency_ns == 0 and target_residency_ns == 0.
    cpu_idx : int
        Logical CPU number this governor instance manages.
    csv_path : str
        Path to the scheduling-event trace CSV file.
    """

    def __init__(self, cpu_states_dict: dict, cpu_idx: int, rows: list):
        self.cpu_states_dict = cpu_states_dict
        self.cpu_idx  = cpu_idx
        self.rows     = rows          # pre-loaded trace rows (shared read-only)

        # Build an ordered list so we can iterate states by index (like drv->states[i]).
        # Normalize keys and units so the governor always works in nanoseconds
        # regardless of whether the caller supplied "exit_latency" (µs) or
        # "exit_latency_ns" (ns).
        self.states = []
        for state in cpu_states_dict.values():
            s = dict(state)   # don't mutate the caller's dict

            # --- key aliasing: accept both bare and _ns suffixed names -------
            for bare, ns_key in (("exit_latency",     "exit_latency_ns"),
                                 ("target_residency",  "target_residency_ns")):
                if ns_key not in s and bare in s:
                    # bare key is in microseconds (matches Linux kernel struct) → convert
                    s[ns_key] = int(s[bare] * NSEC_PER_USEC)

            self.states.append(s)

        # Identify the polling state: exit_latency_ns == 0 AND target_residency_ns == 0.
        # For Skylake this is C0.  Used in _menu_update and menu_select.
        self._polling_idx: int | None = next(
            (i for i, s in enumerate(self.states)
             if s.get("exit_latency_ns", -1) == 0 and s.get("target_residency_ns", -1) == 0),
            None
        )

        self.data = {
            "needs_update":     0,
            "tick_wakeup":      0,
            "next_timer_ns":    0,
            "bucket":           0,
            # Correction factors initialised to unity (RESOLUTION * DECAY).
            "correction_factor": [RESOLUTION * DECAY] * BUCKETS,
            # Recent residency intervals in µs (ring buffer).
            "intervals":        [0] * INTERVALS,
            "interval_ptr":     0,
            # Bookkeeping for menu_update.
            "last_state_idx":       0,
            "last_residency_ns":    0,
            "poll_time_limit":      False,
        }

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _menu_update_intervals(self, interval_us: int):
        """Append one measured residency sample (µs) to the ring buffer."""
        ptr = self.data["interval_ptr"]
        self.data["intervals"][ptr] = interval_us
        self.data["interval_ptr"] = (ptr + 1) % INTERVALS

    def _menu_update(self):
        """
        Update the correction factor for the last used bucket based on the
        measured idle residency.  Mirrors menu_update() in menu.c.
        """
        last_idx  = self.data["last_state_idx"]
        target    = self.states[last_idx]
        exit_lat  = target.get("exit_latency_ns", 0)

        # ----------------------------------------------------------------
        # Determine the measured idle duration (measured_ns).
        # ----------------------------------------------------------------
        if self.data["tick_wakeup"] and self.data["next_timer_ns"] > TICK_NSEC:
            # Woken by a tick that wasn't supposed to fire → predictor was off.
            # Pretend we were idle for 90 % of MAX_INTERESTING.
            measured_ns = 9 * MAX_INTERESTING // 10

        elif (last_idx == self._polling_idx) and self.data["poll_time_limit"]:
            # Polling state exited due to time limit → next timer is the best
            # proxy for what the actual residency should have been.
            measured_ns = self.data["next_timer_ns"]

        else:
            measured_ns = self.data["last_residency_ns"]
            # Deduct exit latency so we measure when the wakeup *began*.
            if measured_ns > 2 * exit_lat:
                measured_ns -= exit_lat
            else:
                measured_ns //= 2

        # Cap at next_timer_ns so the correction factor never exceeds unity.
        if measured_ns > self.data["next_timer_ns"]:
            measured_ns = self.data["next_timer_ns"]

        # ----------------------------------------------------------------
        # Exponential-moving-average update of the correction factor.
        # new_factor = old * (1 - 1/DECAY) + (measured / next_timer) * RESOLUTION
        # ----------------------------------------------------------------
        bucket      = self.data["bucket"]
        new_factor  = self.data["correction_factor"][bucket]
        new_factor -= new_factor // DECAY

        if self.data["next_timer_ns"] > 0 and measured_ns < MAX_INTERESTING:
            new_factor += (RESOLUTION * measured_ns) // self.data["next_timer_ns"]
        else:
            # Idle was so long we treat it as a perfect prediction.
            new_factor += RESOLUTION

        if DECAY == 1 and new_factor == 0:
            new_factor = 1

        self.data["correction_factor"][bucket] = new_factor

        # Store the measured residency (convert ns → µs) in the ring buffer.
        self._menu_update_intervals(measured_ns // NSEC_PER_USEC)

    # ------------------------------------------------------------------
    # Public governor interface
    # ------------------------------------------------------------------

    def menu_reflect(self, index: int, last_residency_ns: int,
                     poll_time_limit: bool = False, tick_wakeup: bool = False):
        """
        Called immediately after the CPU exits an idle state.  Records the
        index and residency so menu_update() can refine the model next time.

        Mirrors menu_reflect() in menu.c.

        Parameters
        ----------
        index             : index of the state that was actually used
        last_residency_ns : how long the CPU was actually idle (ns)
        poll_time_limit   : True if a polling state was exited due to time limit
        tick_wakeup       : True if the wakeup was caused by the scheduler tick
        """
        self.data["last_state_idx"]    = index
        self.data["last_residency_ns"] = last_residency_ns
        self.data["poll_time_limit"]   = poll_time_limit
        self.data["needs_update"]      = 1
        self.data["tick_wakeup"]       = tick_wakeup

    def menu_select(self, timestamp: float, row_idx: int) -> int:
        """
        Choose the deepest idle state the CPU may safely enter at *timestamp*.

        Parameters
        ----------
        timestamp : current time in milliseconds (matches the CSV time column)
        row_idx   : index into self.rows of the <idle> row that triggered this
                    call — used to start the sleep-length scan from the right
                    position without re-reading already-processed rows.

        Returns
        -------
        Index into self.states of the chosen idle state (0 = shallowest).
        """
        data   = self.data
        states = self.states

        # ------------------------------------------------------------------
        # 1. Housekeeping / update from previous idle period.
        # ------------------------------------------------------------------
        if data["needs_update"]:
            self._menu_update()
            data["needs_update"] = 0
        elif data["last_residency_ns"] == 0:
            # Driver rejected the previously selected state – record UINT_MAX
            # so invalid information doesn't skew future predictions.
            self._menu_update_intervals(UINT_MAX)

        # ------------------------------------------------------------------
        # 2. Predict the next wakeup interval.
        # ------------------------------------------------------------------
        predicted_ns = get_typical_interval_ns(data)

        if predicted_ns > RESIDENCY_THRESHOLD_NS or _tick_nohz_tick_stopped():
            # Use the time-to-next-timer as a cross-check / refinement.
            delta = _tick_nohz_get_sleep_length(self.rows, row_idx, self.cpu_idx, timestamp)
            if delta < 0:
                delta      = 0
                delta_tick = 0
            else:
                delta_tick = delta      # tickless: delta_tick == delta

            data["next_timer_ns"] = delta
            data["bucket"]        = which_bucket(data["next_timer_ns"])

            # Apply the correction factor to the timer estimate.
            # timer_us = round_up( next_timer_ns * correction / (RESOLUTION*DECAY) )
            divisor   = RESOLUTION * DECAY * NSEC_PER_USEC
            timer_us  = ((divisor // 2) +
                         data["next_timer_ns"] * data["correction_factor"][data["bucket"]]) // divisor

            # Use whichever prediction is shorter.
            predicted_ns = min(timer_us * NSEC_PER_USEC, predicted_ns)

            # If the tick is already stopped, a mispredicted short idle is
            # expensive (CPU gets stuck in a shallow state).  Use the known
            # timer distance unless the timer fires very soon.
            if (_tick_nohz_tick_stopped()
                    and predicted_ns < TICK_NSEC
                    and data["next_timer_ns"] > SAFE_TIMER_RANGE_NS):
                predicted_ns = data["next_timer_ns"]
        else:
            # Tick is running; we cannot determine next-timer precisely.
            data["next_timer_ns"] = KTIME_MAX
            delta_tick            = TICK_NSEC // 2
            data["bucket"]        = BUCKETS - 1

        # ------------------------------------------------------------------
        # 3. Fast-path: force state[0] when latency budget is exhausted or
        #    the next timer fires before state[1]'s target residency.
        # ------------------------------------------------------------------
        # We have no pmqos latency_req in the simulation; treat it as unlimited
        # (any non-zero large value) so the latency_req == 0 branch is never
        # taken unless the caller explicitly sets it.
        latency_req = getattr(self, "latency_req_ns", KTIME_MAX)

        if latency_req == 0:
            return 0

        if len(states) > 1:
            s1 = states[1]
            if (data["next_timer_ns"] < s1.get("target_residency_ns", 0)
                    or latency_req < s1.get("exit_latency_ns", 0)):
                return 0

        # ------------------------------------------------------------------
        # 4. Walk the state table; pick the deepest state whose constraints
        #    are satisfied.
        # ------------------------------------------------------------------
        idx = -1
        for i, s in enumerate(states):
            if idx == -1:
                idx = i     # first enabled state

            if s.get("exit_latency_ns", 0) > latency_req:
                break

            if s.get("target_residency_ns", 0) <= predicted_ns:
                idx = i
                continue

            # The current state's target residency exceeds predicted_ns.
            # Consider promoting from the polling state (C0) to a real idle
            # state if this state's residency is still below the threshold.
            if (idx == self._polling_idx
                    and s.get("target_residency_ns", 0) < RESIDENCY_THRESHOLD_NS
                    and s.get("target_residency_ns", 0) <= data["next_timer_ns"]
                    and s.get("exit_latency_ns", 0) <= predicted_ns):
                predicted_ns = s["target_residency_ns"]
                idx = i
                break

            if predicted_ns < TICK_NSEC:
                break

            if not _tick_nohz_tick_stopped():
                # Tick is running → shallow state is fine; stay there and let
                # the governor re-evaluate on the next idle entry.
                predicted_ns = states[idx].get("target_residency_ns", 0)
                break

            # Tickless: if the selected state is very shallow and this state's
            # residency fits within the time-to-next-tick, upgrade to it.
            if (states[idx].get("target_residency_ns", 0) < TICK_NSEC
                    and s.get("target_residency_ns", 0) <= delta_tick):
                idx = i

            return idx

        if idx == -1:
            idx = 0     # no states enabled; must use state[0]

        # ------------------------------------------------------------------
        # 5. Possibly retract to a shallower state if the tick will not be
        #    stopped and the winner's residency exceeds delta_tick.
        # ------------------------------------------------------------------
        selected = states[idx]
        if (idx == self._polling_idx or predicted_ns < TICK_NSEC):
            if not _tick_nohz_tick_stopped():
                # Don't stop the tick.
                if idx > 0 and selected.get("target_residency_ns", 0) > delta_tick:
                    # Try to find a shallower state whose residency fits.
                    for i in range(idx - 1, -1, -1):
                        idx = i
                        if states[i].get("target_residency_ns", 0) <= delta_tick:
                            break

        return idx