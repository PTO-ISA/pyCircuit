# ruff: noqa: N802,F841
from pycircuit import module, report, rule

from .types import Word


@module
def DuplicateReportTop():
    value: Word = 7

    @rule
    def observe_duplicate_report():
        report("same", value)
        report("same", value)

    observe_duplicate_report()
