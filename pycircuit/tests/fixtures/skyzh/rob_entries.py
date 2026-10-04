"""One Queue per ROB entry: retire one entry, then flush remaining entries.

Both paths use the same pop source. Clearing independent one-element Queues
does not require a bulk-pop or clear operation on a multi-element FIFO.
"""
from pycircuit import ac


class ROBEntry:
    ready: bool = False
    value: ac.u32 = 0


def initial_entry(index: ac.u32) -> ROBEntry:
    return ROBEntry(ready=index == 0, value=10 + index)


@ac.module
def ROB(entries, head, tail, phase, retired):
    @ac.rule
    def retire_or_flush(slots):
        if phase.value == 0:
            entry = slots[head.value].value
            assert entry.ready
            retired.value = entry.value
            head.value = (head.value + 1) % 4
            phase.value = 1
        else:
            for i in range(4):
                if not slots[i].empty():
                    discarded = slots[i].value
            head.value = 0
            tail.value = 0
            phase.value = 2

    if phase.value < 2:
        retire_or_flush(entries)


@ac.module
def EntryArrayROB():
    entries = ac.array(ac.queue[ROBEntry], shape=(4,), capacity=1, initial=initial_entry)
    head = ac.queue[ac.u32](initial=0)
    tail = ac.queue[ac.u32](initial=0)
    phase = ac.queue[ac.u32](initial=0)
    retired = ac.queue[ac.u32](initial=0)
    rob = ROB(entries, head, tail, phase, retired)
