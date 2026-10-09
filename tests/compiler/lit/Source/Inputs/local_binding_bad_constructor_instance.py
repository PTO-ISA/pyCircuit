from typing import Annotated

from pycircuit import module, rule
from locals.local_binding_child import Word


@module
def Bad(a: Annotated[int, range(1 << 8)]) -> {"result": Annotated[int, range(1 << 8)]}:
    Word = Word()
    other = Word()

    @rule
    def bind_first():
        Word(a=a)

    @rule
    def bind_second():
        other(a=a)

    bind_first()
    bind_second()
    return {"result": other.q}
