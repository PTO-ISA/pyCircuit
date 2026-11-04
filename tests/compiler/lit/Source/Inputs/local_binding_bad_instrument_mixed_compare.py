from typing import Annotated

from pycircuit import log, module, rule


@module
def Bad(a: Annotated[int, range(1 << 8)]) -> {"result": Annotated[int, range(1 << 8)]}:
    state: Annotated[int, range(1 << 8)] = 0
    negative = a - 1
    @rule
    def observe():
        state = a
        log("info", "invalid", negative == 0)
    return {"result": a}
