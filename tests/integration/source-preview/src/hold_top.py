# ruff: noqa: N802,F841
from pycircuit import module, rule

from .types import Word


@module
def HoldTop():
    held: Word = 7

    @rule
    def observe_without_write():
        return

    observe_without_write()
