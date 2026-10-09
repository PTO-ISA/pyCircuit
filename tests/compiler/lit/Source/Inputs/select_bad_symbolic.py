from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << WIDTH)], flag: bool, *, WIDTH: int = 8) -> {"result": Annotated[int, range(1 << 8)]}:
    return {"result": (((a + 1) if flag else 1023) & 255)}
