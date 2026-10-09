from typing import Annotated
from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 8)]
        ) -> {"result": Annotated[int, range(1 << 1)]}:
    return {"result": a & (0 - 1)}
