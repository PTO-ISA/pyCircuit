# ruff: noqa: N802,F841
from pycircuit import module, report, rule

from .types import Word


@module
def ReportProbeTop():
    value: Word = 7

    @rule
    def observe_reports():
        report("first", value)
        report("second", value)

    observe_reports()
