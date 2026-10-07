from typing import Annotated

from pycircuit import dff, module, rule


@module
def CounterLane(  # noqa: N802
    clk: bool, rst: bool,
) -> {"count": Annotated[int, range(1 << 8)]}:  # noqa: F821
    c = dff(T=Annotated[int, range(1 << 8)])

    @rule
    def upd():
        c(clk=clk, rst=rst, d=(c.q + 1) & 255, init=0)

    upd()
    return {"count": c.q}
