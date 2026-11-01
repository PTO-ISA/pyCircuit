"""A small reorder buffer: complete out of order, retire in order."""

# ruff: noqa: F821, F823, F841, F842 -- module-owned state updated inside Rules.
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


@ac.struct
class RobRequest:
    allocate: ac.u1
    allocate_tag: ac.u8
    complete: ac.u1
    complete_index: ac.u2
    complete_tag: ac.u8
    complete_value: ac.u16
    retire: ac.u1
    flush: ac.u1


def update_entry(
    entry: Entry,
    index: ac.u2,
    request: RobRequest,
    head: ac.u2,
    tail: ac.u2,
    result: RobResult,
) -> Entry:
    """Apply the transaction's ordered operations to one committed entry."""
    if request.flush:
        entry = Entry(valid=0)
    else:
        if result.complete_accepted and request.complete_index == index:
            entry = Entry(done=1, tag=entry.tag, value=request.complete_value)
        if result.retire_accepted and head == index:
            entry = Entry(valid=0)
        if result.allocate_accepted and tail == index:
            entry = Entry(tag=request.allocate_tag)
    return entry


@ac.module
def RobStorage() -> None:  # noqa: N802
    entries: ac.table[4, Entry] = Entry(valid=0)
    head: ac.u2 = 0
    tail: ac.u2 = 0
    count: ac.u3 = 0

    @ac.method
    def transact(request: RobRequest) -> RobResult:
        result = RobResult()
        remaining = count
        next_head = head
        next_tail = tail
        if request.flush:
            remaining = 0
            next_head = 0
            next_tail = 0
        else:
            completed = entries[request.complete_index]
            oldest = entries[head]
            if (
                request.complete
                and completed.valid
                and completed.tag == request.complete_tag
            ):
                result.complete_accepted = 1
                if request.complete_index == head:
                    oldest = Entry(
                        done=1, tag=completed.tag, value=request.complete_value
                    )
            if request.retire and remaining != 0 and oldest.done:
                result.retire_accepted = 1
                result.retired_tag = oldest.tag
                result.retired_value = oldest.value
                next_head = head + 1
                remaining = remaining - 1
            if request.allocate and remaining < 4:
                result.allocate_accepted = 1
                result.allocated_index = tail
                next_tail = tail + 1
                remaining = remaining + 1
        result.count = remaining

        # Reads above always observe old Q. Each final write is disjoint; the
        # ordinary locals carry same-cycle completion, retirement and allocation.
        entries[0] = update_entry(entries[0], 0, request, head, tail, result)
        entries[1] = update_entry(entries[1], 1, request, head, tail, result)
        entries[2] = update_entry(entries[2], 2, request, head, tail, result)
        entries[3] = update_entry(entries[3], 3, request, head, tail, result)
        head = next_head
        tail = next_tail
        count = remaining
        return result


@ac.module
def Rob(  # noqa: N802 - hardware module definition
    allocate: ac.u1,
    allocate_tag: ac.u8,
    complete: ac.u1,
    complete_index: ac.u2,
    complete_tag: ac.u8,
    complete_value: ac.u16,
    retire: ac.u1,
    flush: ac.u1,
) -> RobResult:
    storage: RobStorage

    @ac.rule
    def transact() -> RobResult:
        request = RobRequest(
            allocate=allocate,
            allocate_tag=allocate_tag,
            complete=complete,
            complete_index=complete_index,
            complete_tag=complete_tag,
            complete_value=complete_value,
            retire=retire,
            flush=flush,
        )
        return storage.transact(request)

    return transact()
