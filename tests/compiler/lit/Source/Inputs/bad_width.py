from typing import Annotated
from pycircuit import module
from bindings.child import WordLatch


@module
def Top(clk: bool, rst: bool, en: bool,
        data: Annotated[int, range(1 << 3)]) -> {"q": Annotated[int, range(1 << 9)]}:
    child = WordLatch(WIDTH=9)

    child(clk=clk, rst=rst, en=en, data=data)

    return {"q": child.q}
