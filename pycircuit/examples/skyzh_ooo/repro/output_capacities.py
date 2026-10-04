"""Capability request, NOT existing syntax: atomic outputs of sizes 12 and 1."""
from pycircuit import ac


@ac.module
def Probe():
    source = ac.queue[ac.u32](initial=7)

    @ac.rule(capacity=(12, 1))
    def allocate(message):
        value = message.value
        return value, value

    @ac.work
    def work():
        rob, reservation = allocate(source)
