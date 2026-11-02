# ruff: noqa: N802,F841
from pycircuit import autorule, module, report

from .types import Word


@module
def ReportProbeTop():
    value: Word = 7

    @autorule
    def observe_reports():
        report("first", value)
        report("second", value)

    observe_reports()
