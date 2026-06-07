# Intel target residencies: https://github.com/torvalds/linux/blob/master/drivers/idle/intel_idle.c

intel_skylake_c_states = {
    "POLL": {
            "idx": 0, 
            "exit_latency": 0, 
            "target_residency": 0
        },
    "C1": {
            "idx": 1,
            "exit_latency": 2,
            "target_residency": 2
        },
    "C1E": {
            "idx": 2,
            "exit_latency": 10, 
            "target_residency": 20
        },
    "C3": {
            "idx": 3, 
            "exit_latency":70, 
            "target_residency": 100
        },
    "C6": {
            "idx": 4,
            "exit_latency": 85, 
            "target_residency": 200
        },
    "C7s": {
            "idx": 5,
            "exit_latency": 124,
            "target_residency": 800
        },
    "C8": {
            "idx": 6,
            "exit_latency": 200, 
            "target_residency":800
        },
    "C9": {
            "idx": 7,
            "exit_latency":480, 
            "target_residency": 5000
        },
    "C10": {
            "idx": 8,
            "exit_latency": 890, 
            "target_residency": 5000
        }
}

intel_sapphire_rapids_c_states = {
    "POLL": {
            "idx": 0, 
            "exit_latency": 0, 
            "target_residency": 0
        },
    "C1": {
            "idx": 1, 
            "exit_latency": 1, 
            "target_residency": 1
        },
    "C1E": {
            "idx": 2, 
            "exit_latency": 2, 
            "target_residency": 4
        },
    "C6": {
            "idx": 3, 
            "exit_latency": 290, 
            "target_residency": 800
        },
}