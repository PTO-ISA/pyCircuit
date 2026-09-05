"""Parameterized Agentic population count through semantic ACIR/PYC lowering."""

import agentic_circuit as ac


@ac.struct
class Item:
    value: ac.u13
    count: ac.u4


@ac.system
def popcount_pipeline() -> None:
    incoming = ac.source(Item)
    counted = incoming.apply(
        lambda item: item.with_fields(count=ac.popcount(item.value))
    )
    ac.sink(counted)
