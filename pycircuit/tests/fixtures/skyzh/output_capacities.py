"""Independent capacities with a single atomic Rule transaction."""
from pycircuit import ac


@ac.module
def Probe():
    source = ac.queue[ac.u32](initial=7)

    rob = ac.queue[ac.u32](capacity=12)
    reservation = ac.queue[ac.u32](capacity=1)

    @ac.rule
    def allocate(message):
        value = message.value
        return value, value

    rob, reservation = allocate(source)
