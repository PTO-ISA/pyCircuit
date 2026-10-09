from typing import Annotated
from pycircuit import dffe, module, rule


@module
def ZeroLatch(clk: bool, rst: bool, en: bool, data: bool) -> {"q": bool}:
    state = dffe(T=bool)

    @rule
    def bind():
        state(clk=clk, rst=rst, en=en, d=data, init=False)

    bind()
    return {"q": state.q}


@module
def OneLatch(clk: bool, rst: bool, en: bool, data: bool) -> {"q": bool}:
    state = dffe(T=bool)

    @rule
    def bind():
        state(clk=clk, rst=rst, en=en, d=data, init=True)

    bind()
    return {"q": state.q}


@module
def WordLatch(clk: bool, rst: bool, en: bool,
              data: Annotated[int, range(1 << WIDTH)], *, WIDTH: int = 9
              ) -> {"q": Annotated[int, range(1 << WIDTH)]}:
    state = dffe(T=Annotated[int, range(1 << WIDTH)])

    @rule
    def bind():
        state(clk=clk, rst=rst, en=en, d=data, init=0)

    bind()
    return {"q": state.q}
