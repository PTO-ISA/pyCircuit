# ruff: noqa: F821, F842 -- Annotated bindings allocate compiler-owned instances.
import pycircuit as ac
from example_sibling_calls.clients import Reader, Writer
from example_sibling_calls.counter import Counter


@ac.system
def SiblingCalls():  # noqa: N802
    counter: Counter
    writer: Writer
    reader: Reader
    writer(counter)
    reader(counter)
