import pycircuit as ac
from example_sibling_calls.clients import Reader, Writer
from example_sibling_calls.counter import Counter


@ac.system
def SiblingCalls():  # noqa: N802
    counter = Counter()
    Writer(counter)
    Reader(counter)
