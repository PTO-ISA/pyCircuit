from typing import Annotated

from pycircuit import module, rule, autorule
from locals.local_binding_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word()

    @autorule
    def Word():
        assert flag

    child(a=a)

    Word()
    return {"result": child.q}
