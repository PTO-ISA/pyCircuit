from typing import Annotated

from pycircuit import module, rule

from locals.local_binding_child import Word

@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Word()
    value = child.q
    child(a=a)
    return {"result": (value + 1) & 255}
