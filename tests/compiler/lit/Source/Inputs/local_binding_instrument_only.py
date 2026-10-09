from typing import Annotated

from pycircuit import log, module, report, rule


@module
def Instrument(clk: bool, rst: bool, a: Annotated[int, range(1 << 8)], b: Annotated[int, range(1 << 8)], flag: bool, one: Annotated[int, range(1 << 1)]) -> {"result": Annotated[int, range(1 << 8)]}:
    raw = a + 1
    negative = a - b
    condition = flag
    integer = one

    @rule
    def observe():
        log("info", "exact", raw >> 0, negative & 255,
            raw if condition else 256)
        report("integer_kind", integer + 0)

    observe()
    return {"result": a}
