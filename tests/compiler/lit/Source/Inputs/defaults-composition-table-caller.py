"""Consumer depends exclusively on the published nominal provider interface."""
import pycircuit as ac
from defaults_probe.table_provider import Frame, Produce


@ac.module
def Caller(data: ac.u7) -> Frame:  # noqa: N802
    produced = Produce(data)
    return produced
