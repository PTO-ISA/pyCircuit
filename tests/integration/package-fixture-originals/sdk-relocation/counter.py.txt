# ruff: noqa: N802,F841
from pycircuit import log, module, report, rule

from .types import Word


@module
def Counter(incoming: Word, outgoing: Word):
    count: Word = 0

    @rule
    def tick():
        nonlocal count, outgoing
        log("info", "relocation_tick", outgoing)
        report("relocation_count", count)
        outgoing = count
        count = incoming

    tick()
