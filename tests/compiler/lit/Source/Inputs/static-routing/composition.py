"""Provider -> worker -> System is an acyclic source-unit dependency chain."""

# ruff: noqa: F841 -- each construction owns a distinct hardware allocation.
import pycircuit as ac
from static_routes.counter import Counter
from static_routes.worker import Worker


@ac.module
def Parent():
    peer = Counter()
    distinct = Counter()
    caller = Worker()


@ac.system(samples=((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0)))
def Top():
    parent = Parent()
