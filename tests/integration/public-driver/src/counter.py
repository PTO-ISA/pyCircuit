from pycircuit import bits, log, module, report, rule
from source_public.types import CounterResult


@rule
def tick(count, incoming, outgoing) -> CounterResult:
    log("info", "counter_tick", outgoing)
    report("count", count)
    result = CounterResult(count=count)
    count = incoming
    return result


@module
def Counter(incoming: bits[8], outgoing: bits[8]) -> CounterResult:  # noqa: N802
    count: bits[8] = 0
    return tick(count, incoming, outgoing)
