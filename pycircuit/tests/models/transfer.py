from pycircuit import ac

@ac.module
class State:

    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)

    def ready(x: ac.queue[ac.u32]) -> bool:
        return count < 10

    @ac.rule(guard=ready)
    def add(x: ac.queue[ac.u32]) -> ac.u32:
        count = count + x
        return x

    def __call__() -> ac.cell[ac.u32]:
        return count

@ac.module
class Top:

    def __init__() -> None:
        state = State()
        q = ac.queue[ac.u32](initial=3)
        out = ac.queue[ac.u32]()

    @ac.rule
    def run(x: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return state.add(x)

    def __call__() -> ac.queue[ac.u32]:
        state()
        out = run(q)
        return out
