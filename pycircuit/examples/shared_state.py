"""Two mutually exclusive clients share an instance; a third has independent state."""
from pycircuit import ac


@ac.module
def Accumulator():
    total = ac.cell[ac.u32](initial=0)

    @ac.method
    def add(message: ac.queue[ac.u32]) -> ac.u32:
        total.value = total.value + message.value
        return message.value  # Same resource, same outer transaction: one pop.

    return total


@ac.module
def Client(source: ac.queue[ac.u32], accumulator, enabled: bool):
    out = ac.queue[ac.u32]()

    @ac.rule
    def transfer(message: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return accumulator.add(message)  # Retains message's consumption role.

    if enabled:
        out = transfer(source)
    return out


@ac.module
def SharedState(choose_left: bool):
    first = ac.queue[ac.u32](initial=3)
    second = ac.queue[ac.u32](initial=5)
    third = ac.queue[ac.u32](initial=9)
    shared = Accumulator()
    separate = Accumulator()
    left = Client(first, shared, choose_left)
    right = Client(second, shared, not choose_left)
    independent = Client(third, separate, True)
    return shared.outputs, separate.outputs, left.outputs, right.outputs, independent.outputs
