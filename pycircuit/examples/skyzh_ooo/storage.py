"""State owners. Each Queue has one update Rule; control arrives as Signals."""
from .logic import *


def forwarded(slot: Station, execution: Execution) -> Station:
    result = slot
    for i in range(10):
        done = execution.lanes[i]
        if done.complete and done.rob != 0:
            if result.left.tag == done.rob:
                result.left = Operand(value=done.value)
            if result.right.tag == done.rob:
                result.right = Operand(value=done.value)
    return result


@ac.module
def ReservationStation(slot, stage, index, allocation, execution, retirement):
    @ac.rule
    def update(previous) -> Station:
        issue = allocation.value
        events = execution.value
        effect = events.lanes[index]
        commit = retirement.value
        if effect.advance:
            stage.value = effect.state
        if commit.flush:
            stage.value.phase = 0
            if not previous.empty():
                discarded = previous.value
            return None
        if not slot.empty():
            if effect.complete:
                consumed = previous.value
            else:
                updated = forwarded(slot.value, events)
                if index >= 7 and effect.address_valid:
                    updated.address = effect.address
                slot.value = updated
            return None
        for i in range(issue.count):
            incoming = issue.allocations[i]
            if incoming.station == index:
                # Broadcast sees the tag assigned by this cycle's dispatch.
                return forwarded(incoming.slot, events)
        return None
    slot = update(slot)


@ac.module
def ROBSlot(entry, retained_dest, index, allocation, execution, retirement):
    @ac.rule
    def update(previous) -> ROBEntry:
        issue = allocation.value
        events = execution.value
        commit = retirement.value
        incoming = Allocation()
        for i in range(issue.count):
            candidate = issue.allocations[i]
            if candidate.rob == index:
                incoming = candidate
        # skyzh does not clear Dest on retirement/flush. A newly allocated
        # Store retains it until address generation (observable by Load hazard
        # checks). Keep this physical-register detail separate from occupancy.
        destination = retained_dest.value
        if incoming.valid and incoming.entry.ins.opcode != 0x23:
            destination = incoming.entry.dest
        for i in range(4, 7):
            effect = events.lanes[i]
            if effect.rob == index and effect.address_valid:
                destination = effect.address
        retained_dest.value = destination
        if commit.flush or commit.valid and commit.rob == index:
            if not previous.empty():
                discarded = previous.value
            return None
        if incoming.valid:
            value = incoming.entry
            value.dest = destination
            return value
        if not entry.empty():
            value = entry.value
            value.dest = destination
            for i in range(10):
                done = events.lanes[i]
                if done.complete and done.rob == index:
                    value.ready = True
                    value.value = done.value
            entry.value = value
        return None
    entry = update(entry)


@ac.module
def ROBPointers(head, tail, allocation, retirement):
    @ac.rule
    def advance():
        issue = allocation.value
        commit = retirement.value
        if commit.flush:
            head.value = 1
            tail.value = 1
        else:
            tail.value = issue.next_tail
            if commit.valid:
                head.value = next_rob(commit.rob)
    advance()


@ac.module
def RegisterFile(rename, registers, allocation, retirement):
    @ac.rule
    def update():
        issue = allocation.value
        commit = retirement.value
        for i in range(1, 32):
            state = rename[i].value
            if issue.rename and issue.rd == i:
                state = RenameEntry(tag=issue.rename_tag, busy=True)
            if commit.write and commit.entry.dest == i:
                registers[i].value = commit.entry.value
                # Commit tests Reorder.current(): dispatch's new tag wins.
                if state.tag == commit.rob:
                    state.busy = False
            if commit.flush:
                state.busy = False
            rename[i].value = state
    update()


@ac.module
def Memory(memory, retirement):
    @ac.rule
    def write():
        commit = retirement.value
        if commit.store:
            address = commit.entry.dest
            width = 1 << commit.entry.ins.funct3
            assert address + width <= 0x400000 and address % width == 0
            # Combine all bytes of this Store in one word update.
            index = (address >> 2) & 63
            page = address >> 8
            old = memory[page].value.words[index]
            shift = (address & 3) * 8
            mask = ac.u32(0xffffffff) if width == 4 else ((1 << (width * 8)) - 1) << shift
            memory[page].value.words[index] = (old & ~mask) | ((commit.entry.value << shift) & mask)
    write()


@ac.module
def Predictor(predictor, retirement):
    @ac.rule
    def update():
        commit = retirement.value
        if commit.branch:
            address = commit.entry.dest
            history = predictor[address >> 8].value.history[address & 255]
            selected = address + (ac.u32(history) & 3)
            counter = predictor[selected >> 8].value.counters[selected & 255]
            if commit.taken:
                if counter < 3:
                    counter = counter + 1
            elif counter > 0:
                counter = counter - 1
            predictor[selected >> 8].value.counters[selected & 255] = counter
            predictor[address >> 8].value.history[address & 255] = ac.u8((ac.u32(history) << 1) | ac.u32(commit.taken))
    update()
