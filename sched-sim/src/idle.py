from src.states import *

class governor:
    def __init__(self, select_func, reflect_func, memory_dict, states_dict):
        self.select = select_func
        self.reflect = reflect_func
        self.memory = memory_dict
        self.states = states_dict

    def reflect(self):
        self.select(self.memory, self.states_dict)

    def select(self):
        self.reflect(self.memory, self.states_dict)


class idle_governor:
    def __init__(self, governor, cpu_states = intel_skylake_c_states):
        self.governor = governor()
        self.cpu_states = cpu_states

    def schedule(self, miles) -> str:
        self.mileage += miles