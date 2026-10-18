from typing import Annotated

from pycircuit import module
from shifts.shift_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)]) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word(a=(a + 1) >> 1)
    return {"result": child.q}
