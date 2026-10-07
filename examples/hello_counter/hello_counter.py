from pycircuit import bits, log, rule, system


@rule
def increment(count):
    log("info", "count", count)
    count = count + 1


@system
def HelloCounter():  # noqa: N802
    count: bits[8] = 0
    increment(count)
