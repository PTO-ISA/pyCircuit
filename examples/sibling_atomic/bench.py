# ruff: noqa: F823 -- captured System state is compiler-owned.
import pycircuit as ac
from example_sibling_atomic.sibling_atomic import Counter, Ledger, Reader, Writer


@ac.system
def SiblingAtomic():  # noqa: N802
    phase: ac.u3 = 0
    counter = Counter()
    ledger = Ledger()
    Writer(counter, ledger, phase != 1)
    Reader(counter, ledger)

    @ac.rule
    def advance() -> None:
        ac.log("info", "atomic committed", counter.committed(), ledger.read())
        phase = phase + 1
