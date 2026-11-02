from typing import Annotated

from pycircuit import dff, module, rule, autorule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    state = dff(T=bool)
    integer = a & 1
    @autorule
    def update():
        state(clk=flag, rst=False, d=integer, init=False)
    return {"result": a}
