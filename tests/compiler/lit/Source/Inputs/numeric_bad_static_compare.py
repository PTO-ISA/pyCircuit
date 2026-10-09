from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 1)]) -> {"result": bool}:
    return {"result": a < (0 - 1)}
