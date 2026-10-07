from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 1)]) -> {"result": Annotated[int, range(1 << 1)]}:
    return {"result": (0 - 1) ^ a}
