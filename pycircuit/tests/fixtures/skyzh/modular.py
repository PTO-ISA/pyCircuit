"""Small executable probes for readable Module/Queue/Signal composition.

Some tops intentionally fail compilation or demonstrate a semantic boundary.
They do not claim to implement the complete OoO CPU.
"""
from pycircuit import ac


class Request:
    tag: ac.u32 = 0
    value: ac.u32 = 0


class Completion:
    valid: bool = False
    tag: ac.u32 = 0
    value: ac.u32 = 0


class Station:
    tag: ac.u32 = 0
    ready: bool = False
    value: ac.u32 = 0


@ac.signal
def completion(request) -> Completion:
    if request.empty():
        return Completion()
    item = request.value
    return Completion(valid=True, tag=item.tag, value=item.value + 1)


@ac.module
def Execute(request):
    done = completion(request)

    @ac.rule
    def consume(message):
        unused = message.value

    consume(request)
    return done


@ac.module
def ObserveTwo(first, second, result):
    @ac.rule
    def observe():
        a = first.value
        b = second.value
        if a.valid or b.valid:
            result.value = (a.value if a.valid else ac.u32(0)) + (b.value if b.valid else ac.u32(0))
    observe()


@ac.module
def ObserveOne(done, result):
    @ac.rule
    def observe():
        value = done.value
        if value.valid:
            result.value = value.value
    observe()


@ac.module
def ParallelBroadcast():
    first = ac.queue[Request](initial=Request(tag=1, value=10))
    second = ac.queue[Request](initial=Request(tag=2, value=20))
    rob_result = ac.queue[ac.u32](initial=0)
    rs_result = ac.queue[ac.u32](initial=0)
    a = Execute(first)
    b = Execute(second)
    rob = ObserveTwo(a, b, rob_result)
    wake = ObserveTwo(a, b, rs_result)


@ac.module
def Allocate(source, done):
    @ac.rule
    def allocate(message):
        tag = message.value
        slot = Station(tag=tag)
        value = done.value
        if value.valid and value.tag == tag:
            slot.ready = True
            slot.value = value.value
        return slot
    entry = allocate(source)
    return entry


@ac.module
def AllocateBypass():
    incoming = ac.queue[ac.u32](initial=9)
    executing = ac.queue[Request](initial=Request(tag=9, value=40))
    done = Execute(executing)
    entry = Allocate(incoming, done)


@ac.signal
def projection(queue) -> ac.u32:
    return queue.value + 1


@ac.signal
def twice(value) -> ac.u32:
    return value.value * 2


@ac.module
def SignalChain():
    source = ac.queue[ac.u32](initial=3)
    decoded = projection(source)
    selected = twice(decoded)


@ac.module
def CapturedSignal():
    source = ac.queue[ac.u32](initial=3)
    decoded = projection(source)

    @ac.signal
    def selected(queue) -> ac.u32:
        return queue.value + decoded.value

    result = selected(source)


@ac.module
def Compute(source):
    value = source.value + 1
    return value


@ac.module
def WorkOutput():
    source = ac.queue[ac.u32](initial=3)
    value = Compute(source)


@ac.module
def Produce(source):
    @ac.rule
    def produce(message):
        return message.value
    out = produce(source)
    return out


@ac.module
def RuleReturnBroadcast():
    source = ac.queue[Request](initial=Request(tag=9, value=40))
    result = ac.queue[ac.u32](initial=0)
    out = Produce(source)
    done = completion(out)
    sink = ObserveOne(done, result)


@ac.module
def DrainSameRule():
    fifo = ac.queue[ac.u32](capacity=4, initial=[1, 2, 3])

    @ac.rule
    def drain(message):
        unused = message.value

    for i in range(3):
        drain(fifo)


@ac.module
def ClearFIFO():
    fifo = ac.queue[ac.u32](capacity=4, initial=[1, 2, 3])

    @ac.rule
    def flush():
        fifo.clear()

    flush()


@ac.module
def SpeculativeBroadcast():
    source = ac.queue[Request](initial=Request(tag=9, value=40))
    out = ac.queue[Request](initial=Request(tag=1, value=5))
    result = ac.queue[ac.u32](initial=0)
    done = completion(source)
    sink = ObserveOne(done, result)

    @ac.rule
    def produce(message):
        return message.value

    out = produce(source)
