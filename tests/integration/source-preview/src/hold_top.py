# ruff: noqa: N802,F841
from pycircuit import autorule, module

from .types import Word


@module
def HoldTop():
    held: Word = 7

    @autorule
    def observe_without_write():
        return

    observe_without_write()
