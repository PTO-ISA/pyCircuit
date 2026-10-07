from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 1)], b: Annotated[int, range(1 << 1)]) -> {"result": Annotated[int, range(1 << 1)]}:
    return {"result": a if (b * 0) else 0}
