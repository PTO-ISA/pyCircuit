from typing import Annotated
from pycircuit import dff, module, rule


@module
def Bad(a: Annotated[int, range(1 << 13)], b: Annotated[int, range(1 << 13)]) -> {"result": Annotated[int, range(1 << 13)]}:
    return {"result": a + b}
