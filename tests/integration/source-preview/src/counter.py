# ruff: noqa: N802,F841
from pycircuit import autorule, log, module, report

from .types import Word


@module
def Counter(incoming: Word, outgoing: Word):
    count: Word = 0

    @autorule
    def tick():
        nonlocal count, outgoing
        log("info", "counter_tick", outgoing)
        report("count", count)
        outgoing = count
        count = incoming

    tick()
