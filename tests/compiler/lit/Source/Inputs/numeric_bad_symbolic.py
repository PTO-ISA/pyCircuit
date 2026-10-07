from typing import Annotated
from pycircuit import dff, module, rule


@module
def Bad(a: Annotated[int, range(1 << WIDTH)], b: Annotated[int, range(1 << WIDTH)], *, WIDTH: int = 13) -> {"result": Annotated[int, range(1 << WIDTH)]}:
    return {"result": (a + b) & 8191}
