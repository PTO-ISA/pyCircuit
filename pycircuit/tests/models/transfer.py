from pycircuit import ac
@ac.module
def State():
    count = ac.cell[ac.u32](initial=0)
    def ready(x: ac.queue[ac.u32]) -> bool:
        return count.value < 10
    @ac.method(guard=ready)
    def add(x: ac.queue[ac.u32]) -> ac.u32:
        count.value = count.value + x.value
        return x.value
    return count
@ac.module
def Top():
    state = State()
    q = ac.queue[ac.u32](initial=3)
    out = ac.queue[ac.u32]()
    @ac.rule
    def run(x: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return state.add(x)
    out = run(q)
    return out
