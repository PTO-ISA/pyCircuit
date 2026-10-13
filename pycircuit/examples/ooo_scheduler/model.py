"""Two reservation stations, ADD/MUL issue, completion broadcast, ordered commit.

Static construction and explicit selection policy and state ownership.
The transaction and RTL executors verify waiting, broadcast and ordered retirement.
"""
from pycircuit import ac
from payloads import ADD, MUL, MicroOp, Entry, Completion
from state import ReservationStation, ReorderBuffer, RegisterFile
from execution import Scheduler, Adder, Multiplier

@ac.module
class Dispatch:

    def __init__() -> None:
        pass

    @ac.rule
    def dispatch(message: ac.queue[MicroOp]) -> None:
        inst = message
        lhs = rf.operand(inst.rs1)
        rhs = rf.operand(inst.rs2)
        tag = rob.allocate(inst.rd)
        rf.rename(inst.rd, tag)
        entry = Entry(True, inst.kind, tag, inst.rd, lhs, rhs)
        if not left.peek().valid:
            left.place(entry)
        else:
            right.place(entry)

    def __call__(source: ac.queue[MicroOp], rob: ReorderBuffer, rf: RegisterFile, left: ReservationStation, right: ReservationStation, add_done: ac.queue[Completion], mul_done: ac.queue[Completion]) -> None:
        if add_done.empty() and mul_done.empty():
            dispatch(source)

@ac.module
class Broadcast:

    def __init__() -> None:
        pass

    @ac.rule
    def broadcast(a: ac.queue[Completion], b: ac.queue[Completion], choose_add: bool) -> ac.signal[Completion]:
        if choose_add:
            result = a
        else:
            result = b
        rob.complete(result.tag, result.value)
        left.wakeup(result.tag, result.value)
        right.wakeup(result.tag, result.value)
        return result

    def __call__(add_done: ac.queue[Completion], mul_done: ac.queue[Completion], rob: ReorderBuffer, left: ReservationStation, right: ReservationStation) -> ac.signal[Completion]:
        prefer_add = not add_done.empty()
        return broadcast(add_done, mul_done, prefer_add)

@ac.module
class Commit:

    def __init__() -> None:
        retired = ac.cell[ac.u32](initial=0)
        last_tag = ac.cell[ac.u32](initial=0)

    @ac.rule
    def commit() -> None:
        entry = rob.retire()
        rf.commit(entry)
        retired = retired + 1
        last_tag = entry.tag

    def __call__(rob: ReorderBuffer, rf: RegisterFile) -> None:
        commit()

@ac.module
class CompletionAudit:

    def __init__() -> None:
        last_tag = ac.cell[ac.u32](initial=0)

    @ac.rule
    def observe(value: ac.signal[Completion]) -> None:
        saved = value
        last_tag = saved.tag

    def __call__(event: ac.signal[Completion]) -> None:
        observe(event)

@ac.module
class OutOfOrder:

    def __init__() -> None:
        source = ac.queue[MicroOp](capacity=4, initial=[MicroOp(MUL, 1, 2, 3), MicroOp(ADD, 4, 1, 2), MicroOp(ADD, 5, 2, 3), MicroOp(ADD, 6, 4, 5)])
        add_requests = ac.queue[Entry]()
        mul_requests = ac.queue[Entry]()
        add_done = ac.queue[Completion]()
        mul_done = ac.queue[Completion]()
        event = ac.signal[Completion]()
        rob = ReorderBuffer()
        rf = RegisterFile()
        left = ReservationStation()
        right = ReservationStation()
        scheduler = Scheduler()
        adder = Adder()
        multiplier = Multiplier()
        cdb = Broadcast()
        dispatch = Dispatch()
        commit = Commit()
        audit = CompletionAudit()

    def __call__() -> None:
        rf(rob)
        add_requests, mul_requests = scheduler(left, right)
        add_done = adder(add_requests)
        mul_done = multiplier(mul_requests)
        event = cdb(add_done, mul_done, rob, left, right)
        dispatch(source, rob, rf, left, right, add_done, mul_done)
        commit(rob, rf)
        audit(event)
