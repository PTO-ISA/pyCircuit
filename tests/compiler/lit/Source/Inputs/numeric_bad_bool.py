from typing import Annotated
from pycircuit import dff, module, rule


@module
def Bad(a: bool, b: bool) -> {"result": Annotated[int, range(1 << 2)]}:
    return {"result": a + b}
