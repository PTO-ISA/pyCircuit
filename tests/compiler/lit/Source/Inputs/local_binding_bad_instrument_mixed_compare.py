from typing import Annotated

from pycircuit import dff, log, module, rule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    negative = a - 1
    condition = flag
    state = dff(T=Annotated[int, range(1 << 8)])
    @rule
    def observe():
        state(clk=flag, rst=False, d=a, init=0)
        log("info", "invalid", negative == 0)
    observe()
    return {"result": a}
