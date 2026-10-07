from typing import Annotated

from pycircuit import module, rule
from locals.local_binding_child import Word


@module
def Bad(Word: Annotated[int, range(1 << 8)]) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word()

    @rule
    def bind_child():
        child(a=Word)

    bind_child()
    return {"result": child.q}
