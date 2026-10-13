"""Three top choices contrast old Cell feedback, exclusive calls, and a real ring."""
from pycircuit import ac

@ac.module
class CellFeedback:

    def __init__() -> None:
        a = ac.cell[ac.u32](initial=1)
        b = ac.cell[ac.u32](initial=2)

    @ac.rule
    def update_a() -> None:
        a = b + 1

    @ac.rule
    def update_b() -> None:
        b = a + 1

    def __call__() -> tuple[ac.cell[ac.u32], ac.cell[ac.u32]]:
        update_a()
        update_b()
        return (a, b)

@ac.module
class ExclusiveRing:

    def __init__(choose: bool) -> None:
        left = ac.queue[ac.u32](initial=1)
        right = ac.queue[ac.u32](initial=2)

    @ac.rule
    def forward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source

    @ac.rule
    def backward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source

    def __call__() -> tuple[ac.queue[ac.u32], ac.queue[ac.u32]]:
        if choose:
            right = forward(left)
        else:
            left = backward(right)
        return (left, right)

@ac.module
class QueueRing:

    def __init__() -> None:
        left = ac.queue[ac.u32](initial=1)
        right = ac.queue[ac.u32](initial=2)

    @ac.rule
    def forward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source

    @ac.rule
    def backward(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source

    def __call__() -> tuple[ac.queue[ac.u32], ac.queue[ac.u32]]:
        right = forward(left)
        left = backward(right)
        return (left, right)
