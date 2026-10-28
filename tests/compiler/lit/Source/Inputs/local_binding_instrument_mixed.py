from typing import Annotated

from pycircuit import log, module, report, rule
from locals.local_binding_child import WordState


@module
def Instrument(clk: bool, rst: bool, a: Annotated[int, range(1 << 8)], b: Annotated[int, range(1 << 8)], flag: bool, one: Annotated[int, range(1 << 1)]) -> {"result": Annotated[int, range(1 << 8)]}:
    raw = a + 1
    negative = a - b
    condition = flag
    integer = one
    complete = raw & 255
    state = WordState(clk, rst, complete)  # noqa: F841 - retain the storage/instrumentation combination.

    @rule
    def observe():
        log("info", "exact", raw >> 0, negative & 255,
            raw if condition else 256)
        report("integer_kind", integer + 0)

    return {"result": a}
