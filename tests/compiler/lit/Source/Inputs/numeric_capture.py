from typing import Annotated
from pycircuit import dff, dffe, module


@module
def Capture(clk: bool, rst: bool, en: bool,
            a: Annotated[int, range(1 << 8)],
            b: Annotated[int, range(1 << 8)]
            ) -> {"q": Annotated[int, range(1 << 8)],
                  "held": Annotated[int, range(1 << 8)]}:
    state = dff(T=Annotated[int, range(1 << 8)])
    hold = dffe(T=Annotated[int, range(1 << 8)])

    state(clk=clk, rst=rst, d=(a + b) & 255, init=0)

    hold(clk=clk, rst=rst, en=en, d=(a * b) & 255, init=0)

    return {"q": state.q, "held": hold.q}
