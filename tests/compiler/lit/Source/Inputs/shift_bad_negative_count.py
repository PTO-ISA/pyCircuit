from typing import Annotated

from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 8)], b: Annotated[int, range(1 << 8)], flag: bool) -> {"result": Annotated[int, range(1 << 8)]}:
    return {"result": a >> (0 - 1)}
