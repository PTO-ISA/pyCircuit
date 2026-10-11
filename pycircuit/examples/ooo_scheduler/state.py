"""Shared state lives in modules; its methods execute inside the caller's rule."""
from pycircuit import ac
from payloads import Operand, Entry, RobEntry


@ac.module
def ReservationStation():
    slot = ac.cell[Entry](initial=Entry())

    def free(entry: Entry) -> bool:
        return not slot.value.valid

    @ac.method(guard=free)
    def place(entry: Entry):
        slot.value = entry

    def issuable(kind: ac.u32) -> bool:
        entry = slot.value
        return (entry.valid and entry.kind == kind
                and entry.lhs.ready and entry.rhs.ready)

    @ac.method(guard=issuable)
    def take(kind: ac.u32) -> Entry:
        entry = slot.value
        slot.value = Entry()  # Free the slot next cycle.
        return entry

    @ac.method
    def wakeup(tag: ac.u32, value: ac.u32):
        entry = slot.value
        changed = False
        if entry.valid:
            if not entry.lhs.ready and entry.lhs.tag == tag:
                entry.lhs = Operand(True, tag, value)
                changed = True
            if not entry.rhs.ready and entry.rhs.tag == tag:
                entry.rhs = Operand(True, tag, value)
                changed = True
        if changed:
            slot.value = entry

    return slot


@ac.module
def RobSlot():
    state = ac.cell[RobEntry](initial=RobEntry())

    def free(tag: ac.u32, rd: ac.u32) -> bool:
        return not state.value.valid

    @ac.method(guard=free)
    def allocate(tag: ac.u32, rd: ac.u32):
        state.value = RobEntry(True, False, tag, rd, 0)

    @ac.method
    def complete(tag: ac.u32, value: ac.u32):
        entry = state.value
        if entry.valid and not entry.done and entry.tag == tag:
            entry.done = True
            entry.value = value
            state.value = entry

    @ac.method
    def peek() -> RobEntry:
        return state.value

    def completed(tag: ac.u32) -> bool:
        entry = state.value
        return entry.valid and entry.done and entry.tag == tag

    @ac.method(guard=completed)
    def retire(tag: ac.u32) -> RobEntry:
        entry = state.value
        state.value = RobEntry()
        return entry

    return state


@ac.module
def ReorderBuffer():
    # Separate resources make physical slot identity visible in summaries.
    slot0 = RobSlot()
    slot1 = RobSlot()
    slot2 = RobSlot()
    slot3 = RobSlot()
    head = ac.cell[ac.u32](initial=1)
    tail = ac.cell[ac.u32](initial=1)

    @ac.method
    def has_room() -> bool:
        return tail.value - head.value < 4

    def room(rd: ac.u32) -> bool:
        return tail.value - head.value < 4

    @ac.method(guard=room)
    def allocate(rd: ac.u32) -> ac.u32:
        tag = tail.value
        index = (tag - 1) & 3
        if index == 0:
            slot0.allocate(tag, rd)
        elif index == 1:
            slot1.allocate(tag, rd)
        elif index == 2:
            slot2.allocate(tag, rd)
        else:
            slot3.allocate(tag, rd)
        tail.value = tag + 1
        return tag

    @ac.method
    def lookup(tag: ac.u32) -> RobEntry:
        index = (tag - 1) & 3
        if index == 0:
            return slot0.peek()
        elif index == 1:
            return slot1.peek()
        elif index == 2:
            return slot2.peek()
        return slot3.peek()

    @ac.method
    def complete(tag: ac.u32, value: ac.u32):
        index = (tag - 1) & 3
        if index == 0:
            slot0.complete(tag, value)
        elif index == 1:
            slot1.complete(tag, value)
        elif index == 2:
            slot2.complete(tag, value)
        else:
            slot3.complete(tag, value)

    @ac.method
    def retire() -> RobEntry:
        tag = head.value
        index = (tag - 1) & 3
        if index == 0:
            entry = slot0.retire(tag)
        elif index == 1:
            entry = slot1.retire(tag)
        elif index == 2:
            entry = slot2.retire(tag)
        else:
            entry = slot3.retire(tag)
        head.value = tag + 1
        return entry

    return head, tail


@ac.module
def RegisterFile(rob):
    registers = ac.cell[ac.array[ac.u32, 8]](initial=[0, 0, 6, 7, 0, 0, 0, 0])
    renamed = ac.cell[ac.array[ac.u32, 8]](initial=[0, 0, 0, 0, 0, 0, 0, 0])
    committed = ac.cell[ac.array[ac.u32, 8]](initial=[0, 0, 0, 0, 0, 0, 0, 0])

    @ac.method
    def operand(index: ac.u32) -> Operand:
        assert index < 8
        if index == 0:
            return Operand(True, 0, 0)
        tag = renamed.value[index]
        if tag == committed.value[index]:
            return Operand(True, tag, registers.value[index])
        entry = rob.lookup(tag)
        if entry.valid and entry.done and entry.tag == tag:
            return Operand(True, tag, entry.value)
        return Operand(False, tag, 0)

    @ac.method
    def rename(rd: ac.u32, tag: ac.u32):
        assert rd < 8
        if rd != 0:
            renamed.value[rd] = tag

    @ac.method
    def commit(entry: RobEntry):
        if entry.rd != 0:
            registers.value[entry.rd] = entry.value
            committed.value[entry.rd] = entry.tag

    return registers, renamed, committed
