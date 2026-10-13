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

@ac.module
class Client:

    def __init__() -> None:
        pass

    @ac.rule
    def transfer(message: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return accumulator.add(message)

    def __call__(source: ac.queue[ac.u32], accumulator: Accumulator, enabled: bool) -> ac.queue[ac.u32]:
        if enabled:
            return transfer(source)

@ac.module
class SharedState:

    def __init__(choose_left: bool) -> None:
        first = ac.queue[ac.u32](initial=3)
        second = ac.queue[ac.u32](initial=5)
        third = ac.queue[ac.u32](initial=9)
        left_out = ac.queue[ac.u32]()
        right_out = ac.queue[ac.u32]()
        independent_out = ac.queue[ac.u32]()
        shared = Accumulator()
        separate = Accumulator()
        left = Client()
        right = Client()
        independent = Client()

    def __call__() -> None:
        left_out = left(first, shared, choose_left)
        right_out = right(second, shared, not choose_left)
        independent_out = independent(third, separate, True)
