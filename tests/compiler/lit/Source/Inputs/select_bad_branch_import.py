from typing import Annotated
from pycircuit import module, rule
from selects.select_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool
        ) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word()

    @rule
    def bind():
        child(a=a)

    bind()
    return {"result": ((child.q if flag else (a + 1)) & 255)}
