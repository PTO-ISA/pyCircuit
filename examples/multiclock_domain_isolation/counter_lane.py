from typing import Annotated

import pycircuit as ac
from pycircuit import dff, module


@module
def CounterLane(  # noqa: N802
    clk: ac.u1,
    rst: ac.u1,
) -> Annotated[int, range(1 << 8)]:
    c = dff(T=Annotated[int, range(1 << 8)])

    c(clk=clk, rst=rst, d=(c.q + 1) & 255, init=0)

    return c.q
