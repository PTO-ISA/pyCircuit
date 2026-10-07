from typing import Annotated

from pycircuit import log, module, rule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    negative = a - 1
    condition = flag
    @rule
    def observe():
        log("info", "invalid", negative ^ 1)
    observe()
    return {"result": a}
