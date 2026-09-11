from __future__ import annotations

import agentic_circuit as ac


@ac.struct
class Item:
    value: ac.u8
    ordinary: ac.u8
    forced: ac.u8


def add_one(value: ac.u8) -> ac.u8:
    return value + 1


@ac.inline
def choose_increment(value: ac.u8, enabled: bool) -> ac.u8:
    result = value
    if enabled:
        result = add_one(value)
    else:
        result = value + 2
    return result


@ac.system
def pure_helper_pipeline() -> None:
    incoming = ac.source(Item)
    outgoing = incoming.apply(
        lambda item: item.with_fields(
            ordinary=add_one(item.value),
            forced=choose_increment(item.value, True),
        )
    )
    ac.sink(outgoing)
