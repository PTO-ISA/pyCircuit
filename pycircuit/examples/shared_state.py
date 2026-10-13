"""Two mutually exclusive clients share an instance; a third has independent state."""
from pycircuit import ac

@ac.module
class Accumulator:

    def __init__() -> None:
        total = ac.cell[ac.u32](initial=0)

    @ac.rule
    def add(message: ac.queue[ac.u32]) -> ac.u32:
        total = total + message
        return message

    def __call__() -> ac.cell[ac.u32]:
        return total

@ac.module
class Client:

    def __init__() -> None:
        out = ac.queue[ac.u32]()

    @ac.rule
    def transfer(message: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return accumulator.add(message)

    def __call__(source: ac.queue[ac.u32], accumulator: Accumulator, enabled: bool) -> ac.queue[ac.u32]:
        if enabled:
            out = transfer(source)
        return out

@ac.module
class SharedState:

    def __init__(choose_left: bool) -> None:
        first = ac.queue[ac.u32](initial=3)
        second = ac.queue[ac.u32](initial=5)
        third = ac.queue[ac.u32](initial=9)
        shared = Accumulator()
        separate = Accumulator()
        left = Client()
        right = Client()
        independent = Client()

    def __call__() -> tuple[ac.cell[ac.u32], ac.cell[ac.u32], ac.queue[ac.u32], ac.queue[ac.u32], ac.queue[ac.u32]]:
        shared()
        separate()
        left(first, shared, choose_left)
        right(second, shared, not choose_left)
        independent(third, separate, True)
        return (shared.outputs, separate.outputs, left.outputs, right.outputs, independent.outputs)
