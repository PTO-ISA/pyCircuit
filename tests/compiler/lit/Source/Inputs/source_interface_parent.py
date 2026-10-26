from typing import Annotated

from pycircuit import module
from interfaces.source_interface_child import Child


@module
def Parent(x: Annotated[int, range(1 << 8)], unused: Annotated[int, range(1 << 8)]) -> {"y": Annotated[int, range(1 << 8)]}:
    child = Child(N=5)

    child(x=x)
    return {"y": child.y}
