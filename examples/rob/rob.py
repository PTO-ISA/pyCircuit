"""A small reorder buffer: complete out of order, retire in order."""

import pycircuit as ac


@ac.struct
class Entry:
    valid: ac.u1 = 1
    done: ac.u1
    tag: ac.u8
    value: ac.u16


@ac.struct
class RobResult:
    allocate_accepted: ac.u1
    allocated_index: ac.u2
    complete_accepted: ac.u1
    retire_accepted: ac.u1
    retired_tag: ac.u8
    retired_value: ac.u16
    count: ac.u3


@ac.rule
def transact(entries, head, tail, count, allocate, allocate_tag,
             complete, complete_index, complete_tag, complete_value,
             retire, flush) -> RobResult:
    result = RobResult()

    if flush:
        entries = 0
        head = 0
        tail = 0
        count = 0
    else:
        completed = entries[complete_index]
        if complete and completed.valid and completed.tag == complete_tag:
            entries[complete_index] = Entry(
                done=1, tag=completed.tag, value=complete_value)
            result.complete_accepted = 1

        oldest = entries[head]
        if retire and count != 0 and oldest.done:
            result.retire_accepted = 1
            result.retired_tag = oldest.tag
            result.retired_value = oldest.value
            entries[head] = Entry(valid=0)
            head = head + 1
            count = count - 1

        if allocate and count < 4:
            result.allocate_accepted = 1
            result.allocated_index = tail
            entries[tail] = Entry(tag=allocate_tag)
            tail = tail + 1
            count = count + 1

    result.count = count
    return result


@ac.module
def RobStorage(allocate: ac.u1, allocate_tag: ac.u8,  # noqa: N802
               complete: ac.u1, complete_index: ac.u2,
               complete_tag: ac.u8, complete_value: ac.u16,
               retire: ac.u1, flush: ac.u1) -> RobResult:
    # Explicit zero keeps every initial slot invalid despite Entry.valid's default.
    entries = ac.table[4, Entry](init=0)
    head: ac.u2 = 0
    tail: ac.u2 = 0
    count: ac.u3 = 0
    return transact(entries, head, tail, count, allocate, allocate_tag,
                    complete, complete_index, complete_tag, complete_value,
                    retire, flush)


@ac.module
def Rob(allocate: ac.u1, allocate_tag: ac.u8,  # noqa: N802
        complete: ac.u1, complete_index: ac.u2,
        complete_tag: ac.u8, complete_value: ac.u16,
        retire: ac.u1, flush: ac.u1) -> RobResult:
    result = RobStorage(allocate, allocate_tag, complete, complete_index,
                        complete_tag, complete_value, retire, flush)
    return result
