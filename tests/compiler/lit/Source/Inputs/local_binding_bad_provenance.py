from typing import Annotated

from pycircuit import module


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    value = a >> 0
    chain = value
    return {"result": chain ^ 1}
