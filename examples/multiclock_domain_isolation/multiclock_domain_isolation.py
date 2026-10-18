from typing import Annotated

import pycircuit as ac
from pycircuit import module, struct
from example_multiclock_domain_isolation.counter_lane import CounterLane


# (c) Named record that groups one domain's two physical pins. The fields are
# ordered, so the emitted packed layout is clk first, rst second.
@struct
class DomainPins:
    clk: ac.u1
    rst: ac.u1


@module
def MulticlockDomainIsolation(  # noqa: N802
    d_a: DomainPins,
    d_b: DomainPins,
) -> tuple[Annotated[int, range(1 << 8)], Annotated[int, range(1 << 8)]]:
    # (b) Each direct typed call creates one independent CounterLane instance
    # and routes one unchanged physical domain record to its pins.
    a_count = CounterLane(d_a.clk, d_a.rst)
    b_count = CounterLane(d_b.clk, d_b.rst)
    return a_count, b_count
