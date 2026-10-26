# ruff: noqa: F823, F841 -- captured owner references and state updates.
import pycircuit as ac


@ac.struct
class CounterResult:
    value: ac.u8


@ac.struct
class WidthResult:
    small: ac.u4
    large: ac.u8


@ac.rule
def global_increment(value):
    value = value + 3
    return CounterResult(value=value)


@ac.module
def LocalCounter() -> CounterResult:
    count: ac.u8 = 4

    @ac.rule
    def increment(count=count):
        count = count + 1
        return CounterResult(value=count)

    return increment


@ac.module
def GlobalCounter() -> CounterResult:
    count: ac.u8 = 11
    return global_increment(count)


@ac.module
def Widths(small: ac.u4, large: ac.u8) -> WidthResult:
    @ac.rule
    def small_bump(value=small):
        return value + 1

    @ac.rule
    def large_bump(value=large):
        return value + 1

    small_result = small_bump
    large_result = large_bump
    return WidthResult(small=small_result, large=large_result)


@ac.module
def Iterations(start: ac.u8) -> CounterResult:
    @ac.rule
    def fold(value=start, n=3):
        result = value
        for _index in range(n):
            result = result + 2
        return CounterResult(value=result)

    return fold
