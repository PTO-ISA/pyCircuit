from typing import Annotated

from pycircuit import module, rule
from locals.local_binding_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool) -> {"result": Annotated[int, range(1 << 8)]}:
    @rule
    def Word():
        assert flag

    child = Word()

    @rule
    def bind_child():
        child(a=a)

    Word()
    bind_child()
    return {"result": child.q}
