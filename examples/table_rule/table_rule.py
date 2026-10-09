"""Replace one persistent table entry atomically with a queue transfer."""

import pycircuit as ac


@ac.struct
class Entry:
    index: ac.u1
    value: ac.u7


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Entry


@ac.rule
def install(entries, entry: Entry, accept: ac.u1) -> Entry:
    old = entries[entry.index]
    if accept:
        entries[entry.index] = entry
    return old


@ac.module
def TableRule(valid: ac.u1, data: Entry, take: ac.u1) -> Result:  # noqa: N802
    entries = ac.table[2, Entry](init=0)
    ready, available, incoming = ac.queue[Entry](
        valid, data, output_ready, depth=2, ready_policy="downstream_pop"  # noqa: F821
    )
    old = install(entries, incoming, available and output_ready)  # noqa: F821
    output_ready, out_valid, out_data = ac.queue[Entry](
        available, old, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
