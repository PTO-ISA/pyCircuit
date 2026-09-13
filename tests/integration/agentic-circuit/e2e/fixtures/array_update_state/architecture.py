"""Persistent owner consuming an immutable fixed-array update."""

import agentic_circuit as ac


@ac.struct
class Request:
    slot: ac.u1
    index: ac.u8
    replacement: ac.u8
    previous: ac.u8


@ac.struct
class State:
    values: ac.array[5, ac.u8]


@ac.rule
def commit(entries, item):
    old = entries[item.slot]
    index = ac.wrap(item.index, ac.index[5])
    updated = old.values.with_element(index, item.replacement)
    entries[item.slot] = old.with_fields(values=updated)
    return item.with_fields(previous=old.values[index])


@ac.system
def array_update_state(request: Request) -> Request:
    entries: list[State] = [0] * 2
    result = commit(entries, request)
    return result
