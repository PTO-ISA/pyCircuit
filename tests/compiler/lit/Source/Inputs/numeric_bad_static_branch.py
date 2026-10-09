from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 1)], select: bool) -> {"result": Annotated[int, range(1 << 1)]}:
    return {"result": a if select else (0 - 1)}
