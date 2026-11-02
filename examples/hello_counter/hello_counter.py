# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
from pycircuit import autorule, bits, log, system


@system
def HelloCounter():  # noqa: N802
    count: bits[8] = 0

    @autorule
    def increment() -> None:
        log("info", "count", count)
        count = count + 1
