import pycircuit as ac
from pycircuit import log, report, rule


@ac.module
def Top(value: ac.u8, second: ac.u4, ok: bool) -> {"out": ac.u8}:  # noqa: N802, F821 - hardware module and named output annotation
    @rule
    def inspect():
        report("gauge_a", value)
        report("gauge_b", second)
        log("info", "event_pair", value, second)
        log("info", "event_literal", "seed")
    inspect()
    return {"out": value}
