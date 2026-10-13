"""Oldest-ready issue, independent execution queues, and explicit MUL latency."""
from pycircuit import ac
from state import ReservationStation
from payloads import ADD, MUL, Entry, Completion

@ac.module
class Scheduler:

    def __init__() -> None:
        pass

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

    def __call__(left: ReservationStation, right: ReservationStation) -> tuple[ac.queue[Entry], ac.queue[Entry]]:
        a = left.peek()
        b = right.peek()
        a_ready = a.valid and a.lhs.ready and a.rhs.ready
        b_ready = b.valid and b.lhs.ready and b.rhs.ready
        a_add = a_ready and a.kind == ADD
        b_add = b_ready and b.kind == ADD
        a_mul = a_ready and a.kind == MUL
        b_mul = b_ready and b.kind == MUL
        choose_add_left = a_add and (not b_add or a.tag < b.tag)
        choose_mul_left = a_mul and (not b_mul or a.tag < b.tag)
        return (issue_add(choose_add_left), issue_mul(choose_mul_left))

@ac.module
class Adder:

    def __init__() -> None:
        pass

    @ac.rule
    def execute(message: ac.queue[Entry]) -> ac.queue[Completion]:
        entry = message
        return Completion(entry.tag, entry.rd, entry.lhs.value + entry.rhs.value)

    def __call__(source: ac.queue[Entry]) -> ac.queue[Completion]:
        return execute(source)

@ac.module
class Multiplier:

    def __init__() -> None:
        active = ac.cell[bool](initial=False)
        remaining = ac.cell[ac.u32](initial=0)
        request = ac.cell[Entry](initial=Entry())

    @ac.rule
    def accept(message: ac.queue[Entry]) -> None:
        request = message
        remaining = 2
        active = True

    @ac.rule
    def advance() -> ac.queue[Completion]:
        if remaining != 0:
            remaining = remaining - 1
            return None
        entry = request
        active = False
        return Completion(entry.tag, entry.rd, entry.lhs.value * entry.rhs.value)

    def __call__(source: ac.queue[Entry]) -> ac.queue[Completion]:
        busy = active
        if busy:
            return advance()
        else:
            accept(source)
