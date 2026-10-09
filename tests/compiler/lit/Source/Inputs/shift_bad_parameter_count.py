from typing import Annotated

from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 8)], b: Annotated[int, range(1 << 8)], flag: bool, *, count: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    return {"result": a >> count}
