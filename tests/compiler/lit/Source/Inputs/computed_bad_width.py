from typing import Annotated
from pycircuit import module


@module
def Top(a: Annotated[int, range(1 << 13)],
        b: Annotated[int, range(1 << 13)]) -> {"result": bool}:
    return {"result": a ^ b}
