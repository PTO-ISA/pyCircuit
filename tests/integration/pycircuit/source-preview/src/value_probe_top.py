# ruff: noqa: N802,F841,T201
from pycircuit import log, module, rule

from .types import Word


@module
def ValueProbeTop():
    word: Word = 7
    flag: bool = True

    @rule
    def observe_values():
        log("info", "word", word)
        log("info", "flag", flag)
        print("value probe")

    observe_values()
