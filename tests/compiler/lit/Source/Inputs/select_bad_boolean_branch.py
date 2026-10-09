from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool) -> {"result": Annotated[int, range(1 << 10)]}:
    return {"result": True if flag else (a + 1)}
