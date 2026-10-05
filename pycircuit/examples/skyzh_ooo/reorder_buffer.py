"""ROB allocation, result capture, retirement and ring pointers."""
from .types import ac, Allocation, ROBEntry
from .logic import next_rob


@ac.module
def ROBSlot(entry, retained_dest, index, allocation, completion, retirement):
    @ac.rule
    def update(previous) -> ROBEntry:
        issue = allocation.value
        events = completion.value
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
        # Retirement/flush takes priority over allocation and completion.
        if commit.flush or (commit.valid and commit.rob == index):
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
def ReorderBuffer(rob, retained_dest, head, tail, allocation, completion, retirement):
    pointers = ROBPointers(head, tail, allocation, retirement)
    for i in range(8):
        ROBSlot(rob[i], retained_dest[i], i + 1, allocation, completion, retirement)
