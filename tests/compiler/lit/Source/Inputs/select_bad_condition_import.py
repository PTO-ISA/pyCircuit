from typing import Annotated
from pycircuit import module, rule
from selects.select_child import Condition


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool
        ) -> {"result": Annotated[int, range(1 << 8)]}:
    child = Condition()

    @rule
    def bind():
        child(flag=flag)

    bind()
    return {"result": (((a + 1) if child.q else 1023) & 255)}
