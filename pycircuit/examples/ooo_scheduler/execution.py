"""Oldest-ready issue, independent execution queues, and explicit MUL latency."""
from pycircuit import ac
from payloads import ADD, MUL, Entry, Completion


@ac.module
def Scheduler(left, right):
    add_requests = ac.queue[Entry]()
    mul_requests = ac.queue[Entry]()

    @ac.rule
    def issue_add(choose_left: bool) -> ac.queue[Entry]:
        if choose_left:
            return left.take(ADD)
        return right.take(ADD)

    @ac.rule
    def issue_mul(choose_left: bool) -> ac.queue[Entry]:
        if choose_left:
            return left.take(MUL)
        return right.take(MUL)

    a = left.slot.value
    b = right.slot.value
    a_ready = a.valid and a.lhs.ready and a.rhs.ready
    b_ready = b.valid and b.lhs.ready and b.rhs.ready
    a_add = a_ready and a.kind == ADD
    b_add = b_ready and b.kind == ADD
    a_mul = a_ready and a.kind == MUL
    b_mul = b_ready and b.kind == MUL
    choose_add_left = a_add and (not b_add or a.tag < b.tag)
    choose_mul_left = a_mul and (not b_mul or a.tag < b.tag)
    # The selected take method supplies readiness, including when neither
    # station can issue. These calculations only choose the oldest candidate.
    add_requests = issue_add(choose_add_left)
    mul_requests = issue_mul(choose_mul_left)
    return add_requests, mul_requests


@ac.module
def Adder(source: ac.queue[Entry]):
    out = ac.queue[Completion]()

    @ac.rule
    def execute(message: ac.queue[Entry]) -> ac.queue[Completion]:
        entry = message.value
        return Completion(entry.tag, entry.rd, entry.lhs.value + entry.rhs.value)

    out = execute(source)
    return out


@ac.module
def Multiplier(source: ac.queue[Entry]):
    active = ac.cell[bool](initial=False)
    remaining = ac.cell[ac.u32](initial=0)
    request = ac.cell[Entry](initial=Entry())
    out = ac.queue[Completion]()

    @ac.rule
    def accept(message: ac.queue[Entry]):
        request.value = message.value
        remaining.value = 2
        active.value = True

    @ac.rule
    def advance() -> ac.queue[Completion]:
        if remaining.value != 0:
            remaining.value = remaining.value - 1
            return None  # Countdown proceeds without requiring output capacity.
        entry = request.value
        active.value = False
        return Completion(entry.tag, entry.rd, entry.lhs.value * entry.rhs.value)

    busy = active.value
    if busy:
        out = advance()
    else:
        accept(source)
    return out
