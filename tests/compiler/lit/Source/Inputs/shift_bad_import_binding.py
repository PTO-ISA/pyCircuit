from typing import Annotated

from pycircuit import module, rule
from shifts.shift_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)]) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word()

    @rule
    def bind():
        child(a=(a + 1) >> 1)

    bind()
    return {"result": child.q}
