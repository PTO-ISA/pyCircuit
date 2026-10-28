from typing import Annotated

from pycircuit import module, rule
from locals.local_binding_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)], *, Word: int = 0) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word()

    child(a=a)

    return {"result": child.q}
