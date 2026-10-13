"""Two independent Rules use one observation and one shared expression.

This model records sharing in ACIR; it does not implement runtime caching.
"""
from pycircuit import ac

@ac.module
class Observer:

    def __init__() -> None:
        pass

    @ac.rule
    def peek(q: ac.queue[ac.u32]) -> ac.u32:
        return q.peek()

    def __call__() -> None:
        pass

@ac.module
class DualChannel:

    def __init__() -> None:
        observer = Observer()
        source = ac.queue[ac.u32](initial=7)
        left_out = ac.queue[ac.u32]()
        right_out = ac.queue[ac.u32]()

    @ac.rule
    def left(value: ac.u32) -> ac.queue[ac.u32]:
        return value

    @ac.rule
    def right(value: ac.u32) -> ac.queue[ac.u32]:
        return value + 1

    def __call__() -> tuple[ac.queue[ac.u32], ac.queue[ac.u32]]:
        observer()
        shared = observer.peek(source) + 1
        if shared != 0:
            left_out = left(shared)
            right_out = right(shared)
        return (left_out, right_out)
