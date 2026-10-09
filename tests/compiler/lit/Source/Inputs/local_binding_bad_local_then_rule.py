from typing import Annotated

from pycircuit import module, rule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    update = a
    @rule
    def update():
        assert flag
    update()
    return {"result": a}
