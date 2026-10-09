"""An eight-bit counter with data selection and one atomic state update."""

import pycircuit as ac


@ac.struct
class CounterResult:
    count: ac.u8


@ac.rule
def increment(count, enable) -> CounterResult:
    result = CounterResult(count=count)
    count = (count + 1) if enable else count
    return result


@ac.module
def Counter(enable: ac.u1) -> CounterResult:  # noqa: N802
    count: ac.u8 = 0
    return increment(count, enable)
