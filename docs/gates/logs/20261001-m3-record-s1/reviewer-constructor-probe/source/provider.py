from typing import Annotated

Word = Annotated[int, range(256)]
BitWord = Annotated[int, range(18446744073709551616)]

class Pair:
    lo: Word
    hi: bool

    def __init__(self, hi: bool = False, lo: Word = 3):
        self.lo = lo
        self.hi = hi

class Wide:
    low: BitWord
    high: BitWord

    def __init__(self, high: BitWord = 9, low: BitWord = 5):
        self.low = low
        self.high = high

class _Private:
    first: bool
    second: Word

    def __init__(self, second: Word = 7, first: bool = True):
        self.first = first
        self.second = second
