"""Exact-width unsigned fields and bit operations."""

import agentic_circuit as ac


@ac.struct
class MaskedTag:
    value: ac.u13
    mask: ac.u13
    rotated: ac.u13
    sequence: ac.u37


@ac.system
def bit_widths() -> None:
    incoming = ac.source(MaskedTag)
    transformed = incoming.apply(
        lambda item: item.with_fields(
            value=(item.value & item.mask) ^ 1,
            rotated=(item.value << 1) | (item.value >> 12),
        )
    )
    ac.sink(transformed)


specialization = ac.jit(bit_widths)
