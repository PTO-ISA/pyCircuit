# ruff: noqa: N802,F841
from pycircuit import module

from .counter import Counter
from .types import Word


@module
def DesignTop():
    left_input: Word = 3
    right_input: Word = 10
    left_output: Word = 100
    right_output: Word = 200

    left = Counter(left_input, left_output)
    right = Counter(right_input, right_output)
