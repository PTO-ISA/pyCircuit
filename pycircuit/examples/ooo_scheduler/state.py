"""Shared state lives in modules; their Rules execute inside the caller's transaction."""
from pycircuit import ac
from payloads import Operand, Entry, RobEntry

@ac.module
class ReservationStation:

    def __init__() -> None:
        slot = ac.cell[Entry](initial=Entry())

    @ac.rule
    def free(entry: Entry) -> bool:
        return not slot.valid

    @ac.rule(guard=free)
    def place(entry: Entry) -> None:
        slot = entry

    @ac.rule
    def issuable(kind: ac.u32) -> bool:
        entry = slot
        return entry.valid and entry.kind == kind and entry.lhs.ready and entry.rhs.ready

    @ac.rule(guard=issuable)
    def take(kind: ac.u32) -> Entry:
        entry = slot
        slot = Entry()
        return entry

    @ac.rule
    def wakeup(tag: ac.u32, value: ac.u32) -> None:
        entry = slot
        changed = False
        if entry.valid:
            if not entry.lhs.ready and entry.lhs.tag == tag:
                entry.lhs = Operand(True, tag, value)
                changed = True
            if not entry.rhs.ready and entry.rhs.tag == tag:
                entry.rhs = Operand(True, tag, value)
                changed = True
        if changed:
            slot = entry

    @ac.rule
    def peek() -> Entry:
        return slot

@ac.module
class RobSlot:

    def __init__() -> None:
        state = ac.cell[RobEntry](initial=RobEntry())

    @ac.rule
    def free(tag: ac.u32, rd: ac.u32) -> bool:
        return not state.valid

    @ac.rule(guard=free)
    def allocate(tag: ac.u32, rd: ac.u32) -> None:
        state = RobEntry(True, False, tag, rd, 0)

    @ac.rule
    def complete(tag: ac.u32, value: ac.u32) -> None:
        entry = state
        if entry.valid and (not entry.done) and (entry.tag == tag):
            entry.done = True
            entry.value = value
            state = entry

    @ac.rule
    def peek() -> RobEntry:
        return state

    @ac.rule
    def completed(tag: ac.u32) -> bool:
        entry = state
        return entry.valid and entry.done and (entry.tag == tag)

    @ac.rule(guard=completed)
    def retire(tag: ac.u32) -> RobEntry:
        entry = state
        state = RobEntry()
        return entry

@ac.module
class ReorderBuffer:

    def __init__() -> None:
        slots: ac.array(RobSlot, shape=(4,)) = [RobSlot() for i in range(4)]
        head = ac.cell[ac.u32](initial=1)
        tail = ac.cell[ac.u32](initial=1)

    @ac.rule
    def has_room() -> bool:
        return tail - head < 4

    @ac.rule
    def room(rd: ac.u32) -> bool:
        return tail - head < 4

    @ac.rule(guard=room)
    def allocate(rd: ac.u32) -> ac.u32:
        tag = tail
        slots[(tag - 1) & 3].allocate(tag, rd)
        tail = tag + 1
        return tag

    @ac.rule
    def lookup(tag: ac.u32) -> RobEntry:
        return slots[(tag - 1) & 3].peek()

    @ac.rule
    def complete(tag: ac.u32, value: ac.u32) -> None:
        slots[(tag - 1) & 3].complete(tag, value)

    @ac.rule
    def retire() -> RobEntry:
        tag = head
        entry = slots[(tag - 1) & 3].retire(tag)
        head = tag + 1
        return entry

@ac.module
class RegisterFile:

    def __init__(rob: ReorderBuffer) -> None:
        registers = ac.cell[ac.array(ac.u32, shape=(8,))](initial=[0, 0, 6, 7, 0, 0, 0, 0])
        renamed = ac.cell[ac.array(ac.u32, shape=(8,))](initial=[0, 0, 0, 0, 0, 0, 0, 0])
        committed = ac.cell[ac.array(ac.u32, shape=(8,))](initial=[0, 0, 0, 0, 0, 0, 0, 0])

    @ac.rule
    def operand(index: ac.u32) -> Operand:
        assert index < 8
        if index == 0:
            return Operand(True, 0, 0)
        tag = renamed[index]
        if tag == committed[index]:
            return Operand(True, tag, registers[index])
        entry = rob.lookup(tag)
        if entry.valid and entry.done and (entry.tag == tag):
            return Operand(True, tag, entry.value)
        return Operand(False, tag, 0)

    @ac.rule
    def rename(rd: ac.u32, tag: ac.u32) -> None:
        assert rd < 8
        if rd != 0:
            renamed[rd] = tag

    @ac.rule
    def commit(entry: RobEntry) -> None:
        if entry.rd != 0:
            registers[entry.rd] = entry.value
            committed[entry.rd] = entry.tag
