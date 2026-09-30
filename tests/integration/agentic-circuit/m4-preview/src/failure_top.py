# ruff: noqa: N802,F841
from pycircuit import log, module, report, rule

from .types import Word


@module
def FailureTop():
    count: Word = 0

    @rule
    def fail_on_second_tick():
        nonlocal count
        log("info", "failure_probe", count)
        report("last_successful_count", count)
        assert count == 0, "the second registered tick violates the invariant"
        count = count + 1

    fail_on_second_tick()
