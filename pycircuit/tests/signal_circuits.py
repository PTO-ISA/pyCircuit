"""Signal composition with mixed inputs, Module exports and static captures."""
from pycircuit import ac


class Summary:
    total: ac.u32 = 0
    odd: ac.u32 = 0


@ac.signal
def summarize(a, b) -> Summary:
    value = a.value + b.value
    return Summary(total=value, odd=value % 2)


@ac.module
def Source(a, b):
    output = summarize(a, b)
    return output


@ac.signal
def twice(source) -> ac.u32:
    return source.value.total * 2


@ac.signal
def offset(source) -> ac.u32:
    return source.value.total + 10


@ac.signal
def join(left, right, queue) -> ac.u32:
    return left.value + right.value + queue.value


@ac.signal
def parity(source) -> ac.u32:
    return source.value.odd


@ac.signal
def tail(source) -> ac.u32:
    return source.value + 100


@ac.signal
def stable(unused) -> ac.u32:
    return ac.u32(9)


@ac.module
def Drive(a, b):
    @ac.rule
    def change():
        if a.value < 5:
            a.value = a.value + 2
            b.value = 4
    change()


@ac.module
def Observe(value: ac.var[ac.u32], seen):
    @ac.rule
    def record(current):
        seen.value = current
    record(value)


@ac.module
def Filtered(value):
    @ac.rule
    def inspect():
        unused = value.value
    inspect()


@ac.module
def SignalDAG():
    a = ac.queue[ac.u32](initial=1)
    b = ac.queue[ac.u32](initial=2)
    seen = ac.queue[ac.u32](initial=0)
    z_root = Source(a, b)
    # Forward references, and names whose sorted order reverses the DAG.
    a_join = join(b_left, b_right, a)
    b_left = twice(z_root)
    b_right = offset(z_root)
    p_parity = parity(z_root)
    o_tail = tail(p_parity)
    unused = stable(z_root)

    @ac.signal
    def captured(queue) -> ac.u32:
        return queue.value + z_root.value.total

    c_capture = captured(a)
    drive = Drive(a, b)
    observer = Observe(a_join, seen)
    filtered = Filtered(o_tail)


@ac.module
def SignalCycle():
    a = tail(b)
    b = tail(a)


@ac.module
def InvalidSignalArgument():
    a = tail(1)
