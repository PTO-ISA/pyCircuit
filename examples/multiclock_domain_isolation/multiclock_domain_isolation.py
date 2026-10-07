from typing import Annotated

import pycircuit as ac
from pycircuit import module, rule, struct
from example_multiclock_domain_isolation.counter_lane import CounterLane


# (c) Named record that groups one domain's two physical pins. The fields are
# ordered, so the emitted packed layout is clk first, rst second.
@struct
class DomainPins:
    clk: ac.u1
    rst: ac.u1


@module
def MulticlockDomainIsolation(  # noqa: N802
    d_a: DomainPins, d_b: DomainPins,
) -> {
    "a_count": Annotated[int, range(1 << 8)],  # noqa: F821
    "b_count": Annotated[int, range(1 << 8)],  # noqa: F821
}:
    # (b) One reusable lane owns the register and its domain pins. Each lane is
    # instantiated once and bound inside its own rule to a different record.
    lane_a = CounterLane()
    lane_b = CounterLane()

    @rule
    def bind_a():
        lane_a(clk=d_a.clk, rst=d_a.rst)

    @rule
    def bind_b():
        lane_b(clk=d_b.clk, rst=d_b.rst)

    bind_a()
    bind_b()
    return {"a_count": lane_a.count, "b_count": lane_b.count}
