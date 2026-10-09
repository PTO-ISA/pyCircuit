from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 8)]) -> {"result": bool}:
    return {"result": a == 256}
