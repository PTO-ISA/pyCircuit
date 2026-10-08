# ruff: noqa: N802,F841
from typing import Annotated

from pycircuit import module, rule

Word = Annotated[int, range(256)]


@module
def Root():
    value: Word = 3

    @rule
    def tick():
        nonlocal value
        value = (value + 1) & 255

    tick()
