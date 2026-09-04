"""End-to-end Python frontend example for the BaseJump priority encoder.

The queue frontend keeps ``priority_encode`` as a compile-time primitive call.
It is lowered to ``ac.var.priority_encode`` with a packed ``{valid,index}``
result (four bits for an eight-bit input), then consumed by the native PYC and
Verilog compatibility backends.
"""

from agentic_circuit import sink, source, struct, system, u4, u8


def priority_encode(value, *, lo_to_hi=True):
    """Marker call recognized by the Queue Python frontend."""
    return value


@struct
class PriorityItem:
    value: u8
    encoded: u4


@system
def priority_encode_demo() -> None:
    incoming = source(PriorityItem, depth=1, latency=1)
    outgoing = incoming.apply(
        lambda item: item.with_fields(
            encoded=priority_encode(item.value, lo_to_hi=True)
        ),
        depth=1,
        latency=1,
    )
    sink(outgoing)
