from typing import Annotated

from pycircuit import dff, module, rule


@module
def MulticlockRegs(  # noqa: N802
    clk_a: bool,
    rst_a: bool,
    clk_b: bool,
    rst_b: bool,
) -> tuple[Annotated[int, range(1 << 8)], Annotated[int, range(1 << 8)]]:
    a = dff(T=Annotated[int, range(1 << 8)])
    b = dff(T=Annotated[int, range(1 << 8)])

    @rule
    def update_a():
        a(clk=clk_a, rst=rst_a, d=(a.q + 1) & 255, init=0)

    @rule
    def update_b():
        b(clk=clk_b, rst=rst_b, d=(b.q + 1) & 255, init=0)

    update_a()
    update_b()
    return a.q, b.q
