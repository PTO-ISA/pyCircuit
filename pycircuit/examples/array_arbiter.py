"""Array reservation slots with age-matrix arbitration and atomic backpressure."""
from pycircuit import ac

N = 3

class Entry:
    valid: bool = False
    ready: bool = False
    value: ac.u32 = 0

Entries = ac.array(Entry, shape=(N,))
Flags = ac.array(bool, shape=(N,))
Matrix = ac.array(bool, shape=(N, N))

@ac.module
class Slot:
    def __init__() -> None:
        state = ac.cell[Entry](initial=Entry())

    @ac.rule(guard=not state.valid)
    def allocate(value: ac.u32, ready: bool) -> None:
        state = Entry(True, ready, value)

    @ac.rule
    def peek() -> Entry:
        return state

    @ac.rule
    def wake() -> None:
        entry = state
        if entry.valid:
            entry.ready = True
            state = entry

    @ac.rule(guard=state.valid and state.ready)
    def take() -> ac.u32:
        value = state.value
        state = Entry()
        return value

@ac.module
class AgeMatrix:
    def __init__() -> None:
        used = ac.cell[Flags](initial=[False for i in range(N)])
        older = ac.cell[Matrix](initial=[[False for j in range(N)] for i in range(N)])

    @ac.rule
    def allocate(position: ac.u32) -> None:
        flags = used
        matrix = older
        for i in range(N):
            matrix[i, position] = flags[i]
            matrix[position, i] = False
        flags[position] = True
        used = flags
        older = matrix

    @ac.rule
    def release(position: ac.u32) -> None:
        flags = used
        flags[position] = False
        used = flags

    @ac.rule
    def choose(candidates: Entries) -> tuple[bool, ac.u32]:
        mask = candidates.match(lambda entry: True)
        matrix = older
        found = False
        position = 0
        for i in range(N):
            oldest = True
            for j in range(N):
                if mask[j] and matrix[j, i]:
                    oldest = False
            if mask[i] and oldest:
                found = True
                position = i
        return found, position

@ac.module
class ArrayArbiter:
    def __init__() -> None:
        slots: ac.array(Slot, shape=(N,)) = [Slot() for i in range(N)]
        age = AgeMatrix()
        output = ac.queue[ac.u32](initial=99)
        cycle = ac.cell[ac.u32](initial=0)
        issued = ac.cell[ac.u32](initial=0)
        last = ac.cell[ac.u32](initial=0)
        retired = ac.cell[ac.u32](initial=0)

    @ac.rule
    def allocate(position: ac.u32, value: ac.u32, ready: bool) -> None:
        slots[position].allocate(value, ready)
        age.allocate(position)

    @ac.rule
    def issue(candidates: Entries) -> None:
        found, position = age.choose(candidates)
        if found:
            value = slots[position].take()
            age.release(position)
            output = value
            issued = issued + 1

    @ac.rule
    def drain() -> None:
        last = output
        retired = retired + 1

    @ac.rule
    def clock() -> None:
        cycle = cycle + 1

    def __call__() -> None:
        if cycle == 0:
            allocate(2, 20, False)
        elif cycle == 1:
            allocate(0, 10, True)
        elif cycle == 2:
            allocate(1, 30, True)
        entries: Entries = [slots[i].peek() for i in range(N)]
        if cycle >= 3:
            issue(entries[entries.match(lambda entry: entry.valid and entry.ready)])
        if cycle == 5:
            slots[2].wake()
        if cycle >= 4 and cycle % 2 == 0:
            drain()
        clock()
