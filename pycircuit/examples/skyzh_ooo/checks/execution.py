"""Component check, NOT the OoO CPU: replay requests through a blocked pipeline."""
from pycircuit import ac
from ..logic import Request, Control, Completion, load_value, store_value
from ..execute import Integer


@ac.module
def Source(requests):
    index = ac.queue[ac.u32](initial=0)

    @ac.rule
    def produce():
        position = index.value
        if position < len(requests):
            index.value = position + 1
            return requests[position]
        return None

    out = produce()
    return out


@ac.module
def Sink(completed, clock, count, last):
    @ac.rule
    def consume(message):
        last.value = message.value
        count.value = count.value + 1

    if clock.value % 7 >= 4:
        consume(completed)


@ac.module
def Timer(clock):
    @ac.rule
    def tick():
        clock.value = clock.value + 1
    tick()


@ac.module
def ExecutionCheck(requests: ac.vector[Request]):
    control = ac.queue[Control](initial=Control())
    clock = ac.queue[ac.u32](initial=0)
    count = ac.queue[ac.u32](initial=0)
    last = ac.queue[Completion](initial=Completion())
    incoming = Source(requests)
    completed = Integer(incoming, control)
    sink = Sink(completed, clock, count, last)
    timer = Timer(clock)


@ac.module
def MemoryHelpers(word: ac.u32, address: ac.u32, width: ac.u32, value: ac.u32, is_unsigned: bool):
    loaded = ac.queue[ac.u32](initial=0)
    stored = ac.queue[ac.u32](initial=0)
    @ac.rule
    def calculate():
        loaded.value = load_value(word, address, width, is_unsigned)
        stored.value = store_value(word, address, width, value)
    calculate()
