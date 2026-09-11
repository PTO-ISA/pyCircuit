"""One Agentic pipeline covering the shared scalar bit primitives."""

import agentic_circuit as ac


@ac.struct
class Item:
    value: ac.u8
    priority_index: ac.u3
    priority_valid: ac.u1
    population: ac.u4
    leading: ac.u4
    trailing: ac.u4


@ac.system
def bit_primitive_pipeline() -> None:
    incoming = ac.source(Item)
    measured = incoming.apply(
        lambda item: item.with_fields(
            priority_index=ac.priority_encode(item.value).index,
            priority_valid=ac.priority_encode(item.value).valid,
            population=ac.popcount(item.value),
            leading=ac.count_leading_zeros(item.value),
            trailing=ac.count_trailing_zeros(item.value),
        )
    )
    ac.sink(measured)
