"""pyc_reorder_pipeline: hardware fixture; migration notes in tests/compiler/oracles/queue_source/MIGRATION-NOTES.md."""
# ruff: noqa: F821, N802 -- ordinary forward hardware queue-result wires.
import pycircuit as ac


@ac.struct
class Token:
    sequence: ac.u32
    value: ac.u32


@ac.struct
class Entry:
    key: ac.u64
    token: Token
    occupied: ac.u1


@ac.struct
class Move:
    take: ac.u1
    push: ac.u1
    token: Token
    next_key: ac.u64
    fault: ac.u1


@ac.struct
class ReorderResult:
    ready: ac.u1
    available: ac.u1
    head: Token
    next_key: ac.u64
    fault: ac.u1


@ac.rule
def ReorderStep(  # noqa: N802
    entries,
    next_key,
    in_valid,
    in_token,
    out_ready,
) -> Move:
    """One edge of reorder bookkeeping: search / admit / retire, table first-read.

    Every query below reads the committed pre-edge state, and the two writes are
    applied afterwards on disjoint slots (an occupied slot and a free slot can
    never be the same index), so the rule is a total function with no priority
    needed between ``retire`` and ``admit``: the frozen block stages both in the
    same ``doWork`` and commits both in the same ``doXfer`` (``:413-455``).
    """
    match_index, match_valid = entries.first(
        where=lambda entry: entry.occupied and entry.key == next_key
    )
    free_index, free_valid = entries.first(where=lambda entry: not entry.occupied)
    incoming_key: ac.u64 = in_token.sequence
    duplicate_index, duplicate_valid = entries.first(
        where=lambda entry: entry.occupied and entry.key == incoming_key
    )
    stale = incoming_key < next_key
    # `fault` is observation only; `admit` and `retire` below never read it.
    invalid = in_valid and free_valid and (stale or duplicate_valid)
    admit = in_valid and free_valid and not (stale or duplicate_valid)
    retire = match_valid and out_ready
    retired_token: Token = entries[match_index].token
    observed_next_key: ac.u64 = next_key
    if retire:
        entries[match_index].occupied = 0
        next_key = next_key + 1
    if admit:
        entries[free_index].key = incoming_key
        entries[free_index].token = in_token
        entries[free_index].occupied = 1
    return Move(
        take=admit,
        push=retire,
        token=retired_token,
        next_key=observed_next_key,
        fault=invalid,
    )


@ac.module
def ReorderPipeline(  # noqa: N802
    valid: ac.u1, data: Token, take: ac.u1
) -> ReorderResult:
    # `capacity = 16` is the table extent; `start = 0` is `next_key`'s reset value.
    entries = ac.table[16, Entry](init=0)
    next_key: ac.u64 = 0
    # The rule is called FIRST: its arguments forward-reference the future queue
    # results bound below (`language.md:442-443`).  The reverse order is rejected
    # (probe s3: "unknown hardware value 'move'"), and a plain local may not be
    # forward-referenced either ("unknown hardware value 'route_take'").
    move = ReorderStep(entries, next_key, in_valid, in_token, out_ready)
    in_ready, in_valid, in_token = ac.queue[Token](
        valid,
        data,
        move.take,
        depth=8,
        latency=1,
        ready_policy="local_occupancy",
    )
    out_ready, out_valid, out_token = ac.queue[Token](
        move.push,
        move.token,
        take,
        depth=4,
        latency=1,
        ready_policy="local_occupancy",
    )
    return ReorderResult(
        ready=in_ready,
        available=out_valid,
        head=out_token,
        next_key=move.next_key,
        fault=move.fault,
    )
