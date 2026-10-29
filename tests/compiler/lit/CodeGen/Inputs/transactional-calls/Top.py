# ruff: noqa: F821, F842, N802 -- Compiler-owned Module instances.
import pycircuit as ac
from call_test.Accumulator import Accumulator
from call_test.Callers import Callers


@ac.system
def Top():
    shared: Accumulator
    separate: Accumulator
    callers: Callers
    callers(shared, separate)
