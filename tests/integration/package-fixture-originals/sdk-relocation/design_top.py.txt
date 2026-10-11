# ruff: noqa: N802,F841
from pycircuit import module

from .counter import Counter
from .types import Word


@module
def DesignTop():
    incoming: Word = 7
    outgoing: Word = 99
    counter = Counter(incoming, outgoing)
