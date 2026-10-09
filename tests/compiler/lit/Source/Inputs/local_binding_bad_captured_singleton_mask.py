from typing import Annotated

from pycircuit import dff, module, rule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    state = dff(T=Annotated[int, range(1 << 8)])
    mask = 255
    @rule
    def update():
        state(clk=flag, rst=False, d=(a + 1) & mask, init=0)
    update()
    return {"result": state.q}
