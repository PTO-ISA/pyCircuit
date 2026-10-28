from typing import Annotated

from pycircuit import module, rule
from locals.local_binding_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)]) -> {"result": Annotated[int, range(1 << 8)]}:
    Word = Word()
    other = Word()

    Word(a=a)

    other(a=a)

    return {"result": other.q}
