from typing import Annotated

from pycircuit import log, module, rule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool) -> {"result": Annotated[int, range(1 << 8)]}:
    state: Annotated[int, range(1 << 8)] = 0
    condition = flag
    @rule
    def observe():
        state = a
        log("info", "invalid", condition + 1)
    return {"result": a}
