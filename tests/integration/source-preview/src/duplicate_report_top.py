# ruff: noqa: N802,F841
from pycircuit import autorule, module, report

from .types import Word


@module
def DuplicateReportTop():
    value: Word = 7

    @autorule
    def observe_duplicate_report():
        report("same", value)
        report("same", value)

    observe_duplicate_report()
