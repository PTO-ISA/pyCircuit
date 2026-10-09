from measurement_relocated.types import CounterResult
from pycircuit import bits, log, module, report, rule


@rule
def tick(count, incoming, outgoing) -> CounterResult:
    log("info", "relocation_tick", outgoing)
    report("relocation_count", count)
    result = CounterResult(count=count)
    count = incoming
    return result


@module
def Counter(incoming: bits[8], outgoing: bits[8]) -> CounterResult:  # noqa: N802
    count: bits[8] = 0
    return tick(count, incoming, outgoing)
