from typing import Annotated
from pycircuit import module, rule
from selects.select_child import Word


@module
def UnknownPure(a: Annotated[int, range(1 << 8)], flag: bool
                ) -> {"right": Annotated[int, range(1 << 8)],
                      "left": Annotated[int, range(1 << 8)]}:
    child = Word()

    @rule
    def bind():
        child(a=a)

    bind()
    return {"right": child.q if flag else 3, "left": 3 if flag else child.q}
