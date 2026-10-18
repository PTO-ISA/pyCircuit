from typing import Annotated

import pycircuit as ac
from pycircuit import dff, module, rule


@module
def CounterLane(  # noqa: N802
    clk: ac.u1,
    rst: ac.u1,
) -> Annotated[int, range(1 << 8)]:
    c = dff(T=Annotated[int, range(1 << 8)])

    @rule
    def upd():
        c(clk=clk, rst=rst, d=(c.q + 1) & 255, init=0)

    upd()
    return c.q
