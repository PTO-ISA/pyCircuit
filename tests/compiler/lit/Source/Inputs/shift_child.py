from typing import Annotated

from pycircuit import module, struct, u8


@struct
class WordResult:
    q: u8


@module
def Word(a: Annotated[int, range(1 << 8)]) -> WordResult:
    return WordResult(q=a)
