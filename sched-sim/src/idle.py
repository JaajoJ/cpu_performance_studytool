from src.states import *

class governor:
    def __init__(self, select, reflect, memory):
        self.select = select
        self.reflect = reflect
        self.memory = memory

    def reflect(self):
        self.select(self.memory)

    def select(self):
        self.reflect(self.memory)


class idle_governor:
    def __init__(self, governor, cpu_states = intel_skylake_c_states):
        self.governor = governor()
        self.cpu_states = cpu_states

    def schedule(self, miles) -> str:
        self.mileage += miles