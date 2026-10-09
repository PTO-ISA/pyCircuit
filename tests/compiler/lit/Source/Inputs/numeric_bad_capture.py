from typing import Annotated
from pycircuit import dff, module, rule


@module
def Bad(clk: bool, rst: bool, a: Annotated[int, range(1 << 1)]) -> {"result": bool}:
    state = dff(T=bool)

    @rule
    def bind():
        state(clk=clk, rst=rst, d=(a + 0) & 1, init=False)

    bind()
    return {"result": state.q}
