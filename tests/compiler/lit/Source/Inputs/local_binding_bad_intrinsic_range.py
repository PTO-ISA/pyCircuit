from typing import Annotated

from pycircuit import dff, log, module, report, rule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    range = a
    return {"result": a}
