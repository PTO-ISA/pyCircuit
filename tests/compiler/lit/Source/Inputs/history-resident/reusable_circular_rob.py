"""Four-entry resident ROB core; channel owners supply actual boundary availability."""

# ruff: noqa: F841 -- ordinary hardware state proposals.
from pycircuit import module, rule, struct, table, u1, u2, u3, u16


@struct
class RobEvent:
    index: u2
    generation: u16
    epoch: u16
    value: u16
    done: u1


@struct
class RobResult:
    flush_take: u1
    allocate_take: u1
    completion_take: u1
    allocated_valid: u1
    allocated_data: RobEvent
    retired_valid: u1
    retired_data: RobEvent


@rule
def recover(head, tail, count, epoch, grant):
    if grant:
        head = tail
        tail = tail
        count = 0
        epoch = epoch + 1


@rule
def allocate(tail, count, epoch, entries, request: RobEvent, grant) -> RobEvent:
    old = entries[tail]
    allocated = RobEvent(
        index=tail,
        generation=old.generation + 1,
        epoch=epoch,
        value=request.value,
        done=0,
    )
    if grant:
        entries[tail] = allocated
        tail = tail + 1
        count = count + 1
        epoch = epoch
    return allocated


@rule
def complete(entries, index, grant):
    if grant:
        entries[index].done = 1


@rule
def retire(head, count, entries, grant) -> RobEvent:
    old = entries[head]
    if grant:
        entries[head].done = 0
        head = head + 1
        count = count - 1
    return old


@module
def rob(
    flush_available: u1,
    flush_request: RobEvent,
    allocate_available: u1,
    allocate_request: RobEvent,
    completion_available: u1,
    completion: RobEvent,
    allocated_space: u1,
    retired_space: u1,
) -> RobResult:
    head: u2 = 0
    tail: u2 = 0
    count: u3 = 0
    epoch: u16 = 0
    entries = table[4, RobEvent](init=0)
    completion_index = completion.index
    old_completion = entries[completion_index]
    old_head = entries[head]
    flush_fire = flush_available
    allocate_fire = allocate_available & (count != 4) & allocated_space & ~flush_fire
    completion_fire = completion_available & ~flush_fire & ~allocate_fire
    completion_matches = (
        (old_completion.generation == completion.generation)
        & (old_completion.epoch == completion.epoch)
        & (old_completion.epoch == epoch)
    )
    completion_write = completion_fire & completion_matches
    retire_fire = (
        (count != 0)
        & old_head.done
        & retired_space
        & ~flush_fire
        & ~allocate_fire
        & (~completion_write | (completion_index != head))
    )
    recover(head, tail, count, epoch, flush_fire)
    allocated = allocate(tail, count, epoch, entries, allocate_request, allocate_fire)
    complete(entries, completion_index, completion_write)
    retired = retire(head, count, entries, retire_fire)
    return RobResult(
        flush_take=flush_fire,
        allocate_take=allocate_fire,
        completion_take=completion_fire,
        allocated_valid=allocate_fire,
        allocated_data=allocated,
        retired_valid=retire_fire,
        retired_data=retired,
    )
