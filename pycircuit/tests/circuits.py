"""Executable compiler inputs. Never imported by Python tests."""
from pycircuit import ac

class Inner:
    epoch: ac.u16 = 0

class Entry:
    meta: Inner
    value: ac.u8 = 0
    lanes: ac.array[ac.u16, 3]

@ac.module
def Clock(limit, clock):
    @ac.rule
    def tick():
        if clock.value < limit:
            clock.value = clock.value + 1
    @ac.work
    def work():
        tick()

@ac.module
def Atomic():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(10, clock)
    controls = ac.queue[ac.u32](capacity=3, initial=[0, 0, 1])
    left = ac.queue[ac.u32](capacity=2, initial=[5, 6])
    right = ac.queue[ac.u32]()
    total = ac.queue[ac.u32](initial=0)
    count = ac.queue[ac.u32](initial=0)

    @ac.rule
    def route(control, a, alias, b):
        if control.value == 0:
            return a.value + alias.value, ac.u32(1)
        # The control pop must be aborted when the selected b is empty.
        return b.value, None

    @ac.rule
    def consume(a, b):
        total.value = total.value + a.value
        count.value = count.value + b.value

    @ac.work
    def work():
        out, side = route(controls, left, left, right)
        if clock.value >= 4:
            consume(out, side)

@ac.module
def Parameters():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(8, clock)
    entries = [ac.queue[ac.u32](initial=v) for v in [10, 20, 30]]
    total = ac.queue[ac.u32](initial=0)
    count = ac.queue[ac.u32](initial=0)

    @ac.rule(capacity=1)
    def choose(message, bias: ac.var[ac.u32]):
        return message.value + bias

    @ac.rule
    def consume(q):
        total.value = total.value + q.value
        count.value = count.value + 1

    @ac.work
    def work():
        phase = clock.value
        # Two sites share choose's RuleId and its fixed output Queue.
        if phase < 2:
            output = choose(entries[0], phase)
        else:
            output = choose(entries[phase % 3], phase)
        if phase >= 4:
            consume(output)

@ac.signal
def parity(entries) -> ac.u32:
    return ac.u32(entries[0].value.value & 1)

@ac.module
def Observer(shared, observed, shared_again):
    @ac.rule
    def observe():
        observed.value = shared.value
        shared_again.value = shared.value
    @ac.work
    def work():
        observe()

@ac.module
def Configured(initial: ac.vector[ac.u8], amount: ac.u8):
    entries = [ac.queue[Entry](initial=Entry(value=v)) for v in initial]
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(6, clock)
    shared = parity(entries)
    observed = ac.queue[ac.u32](initial=1)
    shared_again = ac.queue[ac.u32](initial=1)
    observer = Observer(shared, observed, shared_again)

    @ac.rule
    def revise(target: ac.ref[Entry], delta: ac.u8):
        target.value.meta.epoch = target.value.meta.epoch + 1
        target.value.value = target.value.value + delta

    @ac.work
    def work():
        n = clock.value
        if n < 6:
            revise(entries[n % len(entries)], amount)

@ac.module
def Events():
    # This event-only Rule has no Queue dependency to wake its Module.
    @ac.rule
    def pulse(delay: ac.u64):
        ac.wakeup(delay)
    @ac.work
    def work():
        pulse(ac.u64(3))

@ac.module
def ShortCircuit():
    empty = ac.queue[ac.u32]()
    counter = ac.queue[ac.u32](initial=0)
    @ac.rule
    def guarded(q):
        if False and q.value == 1:
            counter.value = 99
        if True or q.value == 2:
            counter.value = 7
        return None
        counter.value = q.value
    @ac.work
    def work():
        if counter.value == 7:
            return
        guarded(empty)

@ac.module
def HelperState(value):
    counter = ac.queue[ac.u32](initial=0)
    @ac.rule
    def update():
        counter.value = value
    update()

@ac.module
def Construction():
    queues = [ac.queue[ac.u32](initial=1), ac.queue[ac.u32](initial=2)]
    for i in range(3):
        HelperState(i + 1)
    result = ac.queue[ac.u32](initial=0)
    @ac.rule
    def sum_values():
        acc = ac.u32(0)
        for i in range(2):
            acc = acc + queues[i].value
        result.value = acc
    sum_values()

@ac.module
def Closure():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(4, clock)
    result = ac.queue[ac.u32](initial=0)
    @ac.rule
    def use_local():
        result.value = local_value
    @ac.work
    def work():
        local_value = clock.value + 10
        use_local()

@ac.module
def Arithmetic():
    fixed = ac.queue[ac.array[ac.u16, 3]](initial=[0, 0, 0])
    result = ac.queue[Entry](initial=Entry())
    @ac.rule
    def calculate():
        lanes: ac.array[ac.u16, 3] = [65535, 2, 3]
        lanes[1] = lanes[0] * ac.u16(65535)
        lanes[2] = ac.u16(ac.i16(-7) // ac.i16(3))
        out = Entry(value=ac.u8(255) + ac.u8(2), lanes=lanes)
        out.meta.epoch = ac.u16(ac.i16(-7) % ac.i16(3))
        result.value = out
        fixed.value = lanes
    @ac.work
    def work():
        calculate()

@ac.module
def PairSource():
    @ac.rule(capacity=2)
    def pair():
        ac.wakeup(1)
        return None, ac.u8(7)
    @ac.work
    def work():
        unused, out = pair()
    return unused, out

@ac.module
def PairSink(message):
    total = ac.queue[ac.u32](initial=0)
    @ac.rule
    def accept(q):
        total.value = total.value + ac.u32(q.value)
    @ac.work
    def work():
        accept(message)

@ac.module
def Composed():
    unused, wire = PairSource()
    sink = PairSink(wire)
    array = [ac.queue[ac.u32](initial=i) for i in range(4)]


@ac.module
def DelayedInput(clock):
    @ac.rule
    def produce():
        if clock.value == 2:
            return ac.u32(10)
        return None
    @ac.work
    def work():
        out = produce()
    return out

@ac.module
def AtomicWaiter(first, second, state, marker):
    @ac.rule
    def take(a, b):
        state.value = state.value + 1
        ac.wakeup(20)
        return a.value + b.value
    @ac.rule
    def independent():
        marker.value = 1
    @ac.work
    def work():
        out = take(first, second)
        # An aborted Rule must not stop the rest of Module Work.
        independent()
    return out

@ac.module
def MissingInput():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(3, clock)
    first = ac.queue[ac.u32](capacity=2, initial=[5, 6])
    state = ac.queue[ac.u32](initial=0)
    marker = ac.queue[ac.u32](initial=0)
    second = DelayedInput(clock)
    worker = AtomicWaiter(first, second, state, marker)

@ac.module
def ConditionalWaiter(input, busy, result):
    @ac.rule
    def advance(message):
        if busy.value != 0:
            busy.value = busy.value - 1
        else:
            result.value = message.value
    @ac.work
    def work():
        advance(input)

@ac.module
def ConditionalInput():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(3, clock)
    input = DelayedInput(clock)
    busy = ac.queue[ac.u32](initial=2)
    result = ac.queue[ac.u32](initial=0)
    worker = ConditionalWaiter(input, busy, result)

@ac.module
def ReviseWaiter(input, state):
    @ac.rule
    def assign(target: ac.ref[ac.u32]):
        if state.value == 0:
            state.value = 1
            ac.wakeup(20)
            # No payload read dominates this required revise.
            target.value = 42
    @ac.work
    def work():
        assign(input)

@ac.module
def EmptyRevise():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(3, clock)
    input = DelayedInput(clock)
    state = ac.queue[ac.u32](initial=0)
    worker = ReviseWaiter(input, state)

@ac.module
def ControlWaiter(input, before, after):
    @ac.rule
    def early():
        before.value = before.value + 1
    @ac.rule
    def late(value: ac.u32):
        after.value = value
    @ac.work
    def work():
        early()
        # Stop selection here, preserving early's complete candidate.
        value = input.value
        late(value)

@ac.module
def MissingControl():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(3, clock)
    input = DelayedInput(clock)
    before = ac.queue[ac.u32](initial=0)
    after = ac.queue[ac.u32](initial=0)
    worker = ControlWaiter(input, before, after)

@ac.signal
def optional_input(q) -> ac.u32:
    return ac.u32(0) if q.empty() else q.value

@ac.signal
def required_input(q) -> ac.u32:
    return q.value

@ac.module
def OptionalSignal():
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(3, clock)
    input = DelayedInput(clock)
    value = optional_input(input)

@ac.module
def InvalidSignal():
    input = ac.queue[ac.u32]()
    value = required_input(input)

@ac.signal
def signal_identity(q) -> ac.u32:
    return q.value

@ac.module
def StaticObserver(a, b, phase):
    @ac.rule
    def conditional(shared, enabled: bool):
        if enabled:
            assert shared.value < 100

    @ac.rule
    def selected(shared, enabled: bool):
        if enabled:
            assert shared.value < 100

    @ac.rule
    def captured(enabled: bool):
        if enabled:
            assert b.value < 100

    @ac.rule
    def ordinary(value: ac.u32):
        assert value == 0

    @ac.work
    def work():
        if phase.value < 2:
            conditional(a, False)
        else:
            conditional(b, False)
        alias = a if phase.value < 2 else b
        selected(alias, False)
        captured(False)
        ordinary(b.value & 0)

@ac.signal
def static_branch(selector, entries, bias: ac.u32) -> ac.u32:
    alias = entries[0] if selector.value == 7 else entries[1]
    return alias.value + bias

@ac.module
def StaticSignals():
    fixed = ac.queue[ac.u32](initial=7)
    clock = ac.queue[ac.u32](initial=0)
    timer = Clock(4, clock)
    left = signal_identity(fixed)
    right = signal_identity(clock)
    stable = static_branch(fixed, [fixed, clock], 2)
    observer = StaticObserver(left, right, clock)
