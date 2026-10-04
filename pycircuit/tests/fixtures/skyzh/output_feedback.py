"""Desired allocation: select an empty reservation-station output Queue."""
from pycircuit import ac


@ac.module
def Dispatch(source):
    @ac.rule
    def allocate(message):
        # Selection needs the statically connected output, before any push.
        if first.empty():
            return message.value, None
        return None, message.value

    first, second = allocate(source)

    return first, second


@ac.module
def Probe():
    source = ac.queue[ac.u32](initial=7)
    first, second = Dispatch(source)
