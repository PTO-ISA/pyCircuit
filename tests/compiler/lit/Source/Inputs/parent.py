from typing import Annotated
from pycircuit import module
from bindings.child import OneLatch, WordLatch, ZeroLatch


@module
def Top(clk: bool, rst: bool, en: bool, bit_data: bool,
        word_data: Annotated[int, range(1 << WIDTH)], *, WIDTH: int = 9
        ) -> {"zero": bool, "one": bool,
              "word": Annotated[int, range(1 << WIDTH)]}:
    zero = ZeroLatch()
    one = OneLatch()
    word = WordLatch(WIDTH=WIDTH)

    zero(clk=clk, rst=rst, en=en, data=bit_data)

    one(clk=clk, rst=rst, en=en, data=bit_data)

    word(clk=clk, rst=rst, en=en, data=word_data)

    return {"zero": zero.q, "one": one.q, "word": word.q}
