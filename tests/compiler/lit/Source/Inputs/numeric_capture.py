from typing import Annotated
from pycircuit import dff, dffe, module, rule


@module
def Capture(clk: bool, rst: bool, en: bool,
            a: Annotated[int, range(1 << 8)],
            b: Annotated[int, range(1 << 8)]
            ) -> {"q": Annotated[int, range(1 << 8)],
                  "held": Annotated[int, range(1 << 8)]}:
    state = dff(T=Annotated[int, range(1 << 8)])
    hold = dffe(T=Annotated[int, range(1 << 8)])

    @rule
    def bind_state():
        state(clk=clk, rst=rst, d=(a + b) & 255, init=0)

    @rule
    def bind_hold():
        hold(clk=clk, rst=rst, en=en, d=(a * b) & 255, init=0)

    bind_state()
    bind_hold()
    return {"q": state.q, "held": hold.q}
