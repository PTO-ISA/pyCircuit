import pycircuit as ac
from example_sibling_calls.counter import Counter


@ac.module
def Writer(counter: Counter):  # noqa: N802
    @ac.rule
    def write() -> None:
        counter.increment()


@ac.module
def Reader(counter: Counter):  # noqa: N802
    @ac.rule
    def observe() -> None:
        value = counter.read()
        ac.log("info", "count", value)
