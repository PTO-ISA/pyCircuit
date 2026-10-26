from typing import Annotated
from pycircuit import module
from selects.select_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool
        ) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word()

    child(a=a)

    return {"result": ((child.q if flag else (a + 1)) & 255)}
