"""Two static priority-encoder configurations used by ``run_variants.py``.

The queue frontend intentionally requires ``lo_to_hi`` to be a compile-time
boolean.  Each system below therefore represents one elaborated configuration:
the first is an 8-bit low-to-high encoder and the second is a 16-bit
high-to-low encoder.  The same Python primitive call lowers to different ACIR
and PYC widths/attributes for each system.
"""

from agentic_circuit import sink, source, struct, system, u4, u5, u8, u16


def priority_encode(value, *, lo_to_hi=True):
    """Frontend marker lowered to ``ac.var.priority_encode``."""
    return value


@struct
class PriorityItemW8:
    value: u8
    encoded: u4


@system
def priority_encode_demo_w8_lo() -> None:
    incoming = source(PriorityItemW8, depth=1, latency=1)
    outgoing = incoming.apply(
        lambda item: item.with_fields(
            encoded=priority_encode(item.value, lo_to_hi=True)
        ),
        depth=1,
        latency=1,
    )
    sink(outgoing)


@struct
class PriorityItemW16:
    value: u16
    encoded: u5


@system
def priority_encode_demo_w16_hi() -> None:
    incoming = source(PriorityItemW16, depth=1, latency=1)
    outgoing = incoming.apply(
        lambda item: item.with_fields(
            encoded=priority_encode(item.value, lo_to_hi=False)
        ),
        depth=1,
        latency=1,
    )
    sink(outgoing)
