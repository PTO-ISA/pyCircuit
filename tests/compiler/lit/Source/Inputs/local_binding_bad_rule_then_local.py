from typing import Annotated

from pycircuit import module, rule


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    @rule
    def update():
        assert flag
    update = a
    update()
    return {"result": a}
