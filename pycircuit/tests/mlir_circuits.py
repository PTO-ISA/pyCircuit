from pycircuit import ac


class Packet:
    lanes: ac.array[ac.u32, 4]
    tag: ac.u32 = 0


def accumulate(count: ac.u32) -> Packet:
    result = Packet()
    for i in range(count):
        for j in range(4):
            if j != 2:
                result.lanes[j] = result.lanes[j] + i + j
        result.tag = result.tag + 1
    return result


@ac.signal
def aggregate_signal(count) -> Packet:
    result = accumulate(count.value)
    for i in range(count.value):
        result.lanes[2] = result.lanes[2] + i
    return result


@ac.module
def AggregateLoop(count: ac.u32):
    iterations = ac.queue[ac.u32](initial=count)
    output = aggregate_signal(iterations)


def initial(index: ac.u32) -> Packet:
    return Packet(lanes=[index, index + 1, index + 2, index + 3])


@ac.module
def Forward():
    source = ac.queue[ac.u32](capacity=4, initial=[3, 5, 7])
    rob = ac.queue[ac.u32](capacity=12)
    station = ac.queue[ac.u32](capacity=1)
    @ac.rule
    def allocate(q):
        if rob.size() == 0 and station.empty():
            return q.value, q.value
        return None, None
    if station.empty():
        rob, station = allocate(source)


@ac.module
def Implicit():
    source = ac.queue[ac.u32](initial=9)
    @ac.rule
    def allocate(q):
        if output.empty():
            return q.value
        return None
    if output.empty():
        output = allocate(q=source)
    return output


@ac.module
def Arrays(index: ac.u32):
    slots = ac.array(ac.queue[Packet], shape=(4,), initial=initial)
    empty_slots = ac.array(ac.queue[ac.u32], shape=(3,))
    record = ac.queue[Packet](capacity=2, initial=[Packet(lanes=[1, 2, 3, 4]), Packet(lanes=[10, 20, 30, 40])])
    done = ac.queue[bool](initial=False)
    @ac.rule
    def update():
        if not done.value:
            for i in range(4):
                slots[i].value.lanes[index] = i + 100
            record.value.lanes[index] = 99
            record.value.tag = 5
            done.value = True
    update()


@ac.module
def Keywords(unsigned: ac.u32):
    switch = ac.queue[ac.u32](initial=0)
    @ac.rule
    def operator(template: ac.var[ac.u32]):
        switch.value = template + unsigned
    operator(7)


@ac.signal
def enabled(q) -> bool:
    return q.value != 0


@ac.module
def Consumer(source: ac.queue[ac.u32], enable: ac.var[bool], seen):
    @ac.rule
    def consume(q):
        if enable:
            seen.value = q.value
    if enable:
        consume(source)


@ac.module
def SignalVar():
    state = ac.queue[ac.u32](initial=1)
    source = ac.queue[ac.u32](initial=42)
    seen = ac.queue[ac.u32](initial=0)
    ready = enabled(state)
    consumer = Consumer(source, ready, seen)


@ac.module
def NestedOutputs():
    source = ac.queue[ac.u32](initial=4)
    first = ac.queue[ac.u32](capacity=3)
    second = ac.queue[ac.u32](capacity=2)
    third = ac.queue[ac.u32](capacity=1)
    @ac.rule
    def split(q):
        return (q.value, [None, q.value + 1])
    first, [second, third] = split(source)


@ac.module
def TemporaryRefs():
    left = ac.queue[ac.u32](initial=10)
    right = ac.queue[ac.u32](initial=20)
    output = ac.queue[ac.u32](initial=1)
    clock = ac.queue[ac.u32](initial=0)
    total = ac.queue[ac.u32](initial=0)
    @ac.rule
    def tick():
        clock.value = clock.value + 1
    @ac.rule
    def produce(items):
        return items[0].value + items[1].value
    @ac.rule
    def drain(message):
        total.value = total.value + message.value
    tick()
    output = produce([left, right])
    if clock.value >= 3:
        drain(output)


@ac.module
def StaticPorts():
    values = ac.array(ac.queue[ac.u32], shape=(2,), initial=7)
    unused = ac.queue[ac.u32](initial=0)
    @ac.signal
    def first(items, extra) -> ac.u32:
        return items[0].value
    observed = first(values, unused)
    @ac.rule
    def change():
        if unused.value == 0:
            unused.value = 1
        else:
            values[1].value = values[1].value + 1
    change()
