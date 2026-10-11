"""Two reservation stations, ADD/MUL issue, completion broadcast, ordered commit.

No dynamic construction, executable Python, scheduler backend, or speculation.
Selection policy and state ownership are expressed explicitly in source.
"""
from pycircuit import ac
from payloads import ADD, MUL, MicroOp, Entry, Completion
from state import ReservationStation, ReorderBuffer, RegisterFile
from execution import Scheduler, Adder, Multiplier


@ac.module
def Dispatch(source: ac.queue[MicroOp], rob, rf, left, right,
             add_done: ac.queue[Completion], mul_done: ac.queue[Completion]):
    @ac.rule
    def dispatch(message: ac.queue[MicroOp]):
        inst = message.value
        lhs = rf.operand(inst.rs1)  # Read mappings before renaming destination.
        rhs = rf.operand(inst.rs2)
        tag = rob.allocate(inst.rd)
        rf.rename(inst.rd, tag)
        entry = Entry(True, inst.kind, tag, inst.rd, lhs, rhs)
        if not left.slot.value.valid:
            left.place(entry)
        else:
            right.place(entry)

    # Avoid inserting an unready operand during its producer's wakeup cycle.
    # Readiness is next-cycle Cell state, with no completion-to-issue bypass.
    # Input consumption, ROB allocation and the selected place method supply
    # their own readiness; only this scheduling policy needs an explicit check.
    if add_done.empty() and mul_done.empty():
        dispatch(source)


@ac.module
def Broadcast(add_done: ac.queue[Completion], mul_done: ac.queue[Completion],
              rob, left, right):
    event = ac.signal[Completion](initial=Completion())
    prefer_add = not add_done.empty()  # Shared preparation SSA selects one queue.

    @ac.rule
    def broadcast(a: ac.queue[Completion], b: ac.queue[Completion], choose_add: bool) -> ac.signal[Completion]:
        if choose_add:
            result = a.value
        else:
            result = b.value
        # Exactly one input pop, one selected ROB slot, and both RS wakeups
        # belong to this single outer transaction.
        rob.complete(result.tag, result.value)
        left.wakeup(result.tag, result.value)
        right.wakeup(result.tag, result.value)
        return result

    event = broadcast(add_done, mul_done, prefer_add)
    return event


@ac.module
def Commit(rob, rf):
    retired = ac.cell[ac.u32](initial=0)
    last_tag = ac.cell[ac.u32](initial=0)

    @ac.rule
    def commit():
        entry = rob.retire()  # Nested method guard requires completed old head.
        rf.commit(entry)
        retired.value = retired.value + 1
        last_tag.value = entry.tag

    commit()
    return retired, last_tag


@ac.module
def CompletionAudit(event: ac.signal[Completion]):
    last_tag = ac.cell[ac.u32](initial=0)

    @ac.rule
    def observe(value: ac.signal[Completion]):
        saved = value
        last_tag.value = saved.tag

    observe(event)  # Observe retained completion; a read is not a new event.
    return last_tag


@ac.module
def OutOfOrder():
    source = ac.queue[MicroOp](capacity=4, initial=[
        MicroOp(MUL, 1, 2, 3),  # tag 1: x1 = 6 * 7
        MicroOp(ADD, 4, 1, 2),  # tag 2: waits for tag 1
        MicroOp(ADD, 5, 2, 3),  # tag 3: ready independently of tag 1
        MicroOp(ADD, 6, 4, 5),  # tag 4: waits for tags 2 and 3
    ])
    rob = ReorderBuffer()
    rf = RegisterFile(rob)
    left = ReservationStation()
    right = ReservationStation()
    scheduler = Scheduler(left, right)
    adder = Adder(scheduler.outputs[0])
    multiplier = Multiplier(scheduler.outputs[1])
    cdb = Broadcast(adder.outputs, multiplier.outputs, rob, left, right)
    dispatch = Dispatch(source, rob, rf, left, right, adder.outputs, multiplier.outputs)
    commit = Commit(rob, rf)
    audit = CompletionAudit(cdb.outputs)
    return rf.outputs, rob.outputs, commit.outputs, audit.outputs
