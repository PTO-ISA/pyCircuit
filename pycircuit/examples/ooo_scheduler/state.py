"""Shared state lives in modules; their Rules execute inside the caller's transaction."""
from pycircuit import ac
from payloads import Operand, Entry, RobEntry

@ac.module
class ReservationStation:

    def __init__() -> None:
        slot = ac.cell[Entry](initial=Entry())

    def free(entry: Entry) -> bool:
        return not slot.valid

    @ac.rule(guard=free)
    def place(entry: Entry) -> None:
        slot = entry

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
        slot0 = RobSlot()
        slot1 = RobSlot()
        slot2 = RobSlot()
        slot3 = RobSlot()
        head = ac.cell[ac.u32](initial=1)
        tail = ac.cell[ac.u32](initial=1)

    @ac.rule
    def has_room() -> bool:
        return tail - head < 4

    def room(rd: ac.u32) -> bool:
        return tail - head < 4

    @ac.rule(guard=room)
    def allocate(rd: ac.u32) -> ac.u32:
        tag = tail
        index = tag - 1 & 3
        if index == 0:
            slot0.allocate(tag, rd)
        elif index == 1:
            slot1.allocate(tag, rd)
        elif index == 2:
            slot2.allocate(tag, rd)
        else:
            slot3.allocate(tag, rd)
        tail = tag + 1
        return tag

    @ac.rule
    def lookup(tag: ac.u32) -> RobEntry:
        index = tag - 1 & 3
        if index == 0:
            return slot0.peek()
        elif index == 1:
            return slot1.peek()
        elif index == 2:
            return slot2.peek()
        return slot3.peek()

    @ac.rule
    def complete(tag: ac.u32, value: ac.u32) -> None:
        index = tag - 1 & 3
        if index == 0:
            slot0.complete(tag, value)
        elif index == 1:
            slot1.complete(tag, value)
        elif index == 2:
            slot2.complete(tag, value)
        else:
            slot3.complete(tag, value)

    @ac.rule
    def retire() -> RobEntry:
        tag = head
        index = tag - 1 & 3
        if index == 0:
            entry = slot0.retire(tag)
        elif index == 1:
            entry = slot1.retire(tag)
        elif index == 2:
            entry = slot2.retire(tag)
        else:
            entry = slot3.retire(tag)
        head = tag + 1
        return entry

@ac.module
class RegisterFile:

    def __init__() -> None:
        registers = ac.cell[ac.array[ac.u32, 8]](initial=[0, 0, 6, 7, 0, 0, 0, 0])
        renamed = ac.cell[ac.array[ac.u32, 8]](initial=[0, 0, 0, 0, 0, 0, 0, 0])
        committed = ac.cell[ac.array[ac.u32, 8]](initial=[0, 0, 0, 0, 0, 0, 0, 0])

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

    def __call__(rob: ReorderBuffer) -> None:
        pass
