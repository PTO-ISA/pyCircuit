"""A small reorder buffer: complete out of order, retire in order."""

# ruff: noqa: F823, F841 -- module-owned state updated inside Rules.
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


@ac.struct
class Allocation:
    accepted: ac.u1
    index: ac.u2
    count: ac.u3


@ac.struct
class Retirement:
    accepted: ac.u1
    tag: ac.u8
    value: ac.u16


@ac.module
def RobStorage(request: RobRequest) -> RobResult:  # noqa: N802
    # Explicit zero keeps every initial slot invalid despite Entry.valid's default.
    entries = ac.table[4, Entry](init=0)
    head: ac.u2 = 0
    tail: ac.u2 = 0
    count: ac.u3 = 0

    @ac.method
    def flush() -> None:
        if request.flush:
            entries = 0
            head = 0
            tail = 0
            count = 0

    @ac.method
    def complete() -> ac.u1:
        accepted: ac.u1 = 0
        completed = entries[request.complete_index]
        if (
            not request.flush
            and request.complete
            and completed.valid
            and completed.tag == request.complete_tag
        ):
            entries[request.complete_index] = Entry(
                done=1, tag=completed.tag, value=request.complete_value
            )
            accepted = 1
        return accepted

    @ac.method
    def retire() -> Retirement:
        result = Retirement()
        oldest = entries[head]
        if not request.flush and request.retire and count != 0 and oldest.done:
            result.accepted = 1
            result.tag = oldest.tag
            result.value = oldest.value
            entries[head] = Entry(valid=0)
            head = head + 1
            count = count - 1
        return result

    @ac.method
    def allocate() -> Allocation:
        result = Allocation()
        if not request.flush and request.allocate and count < 4:
            result.accepted = 1
            result.index = tail
            entries[tail] = Entry(tag=request.allocate_tag)
            tail = tail + 1
            count = count + 1
        result.count = count
        return result

    @ac.rule
    def transact() -> RobResult:
        flush()
        completed = complete()
        retired = retire()
        allocated = allocate()
        return RobResult(
            allocate_accepted=allocated.accepted,
            allocated_index=allocated.index,
            complete_accepted=completed,
            retire_accepted=retired.accepted,
            retired_tag=retired.tag,
            retired_value=retired.value,
            count=allocated.count,
        )

    return transact


@ac.module
def Rob(  # noqa: N802 - hardware module definition
    allocate: ac.u1,
    allocate_tag: ac.u8,  # noqa: N802
    complete: ac.u1,
    complete_index: ac.u2,
    complete_tag: ac.u8,
    complete_value: ac.u16,
    retire: ac.u1,
    flush: ac.u1,
) -> RobResult:
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
    return RobStorage(request)
