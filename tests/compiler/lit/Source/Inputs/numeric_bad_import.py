from typing import Annotated
from pycircuit import module, rule
from numeric.numeric_child import Child


@module
def Bad(x: Annotated[int, range(1 << 13)]
        ) -> {"result": Annotated[int, range(1 << 13)]}:
    child = Child()

    @rule
    def bind():
        child(x=x)

    bind()
    return {"result": (child.q + 1) & 8191}
