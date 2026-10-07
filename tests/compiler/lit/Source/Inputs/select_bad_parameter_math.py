from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool,
        *, VALUE: int = 255) -> {"result": Annotated[int, range(1 << 8)]}:
    return {"result": (((a + 1) if flag else VALUE) & 255)}
