"""Agentic leading/trailing zero count through one semantic family."""

import agentic_circuit as ac


@ac.struct
class Item:
    value: ac.u13
    leading: ac.u4
    trailing: ac.u4


@ac.system
def count_zeros_pipeline() -> None:
    incoming = ac.source(Item)
    counted = incoming.apply(
        lambda item: item.with_fields(
            leading=ac.count_leading_zeros(item.value),
            trailing=ac.count_trailing_zeros(item.value),
        )
    )
    ac.sink(counted)
