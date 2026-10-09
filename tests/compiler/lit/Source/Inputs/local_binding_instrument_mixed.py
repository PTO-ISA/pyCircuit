from typing import Annotated

from pycircuit import dff, log, module, report, rule


@module
def Instrument(clk: bool, rst: bool, a: Annotated[int, range(1 << 8)], b: Annotated[int, range(1 << 8)], flag: bool, one: Annotated[int, range(1 << 1)]) -> {"result": Annotated[int, range(1 << 8)]}:
    raw = a + 1
    negative = a - b
    condition = flag
    integer = one
    state = dff(T=Annotated[int, range(1 << 8)])
    complete = raw & 255

    @rule
    def observe():
        state(clk=clk, rst=rst, d=complete, init=0)
        log("info", "exact", raw >> 0, negative & 255,
            raw if condition else 256)
        report("integer_kind", integer + 0)

    observe()
    return {"result": a}
