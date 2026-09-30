# ruff: noqa: N802,F841
from pycircuit import module

from .types import Word


@module
def ZeroRuleTop():
    retained: Word = 9
