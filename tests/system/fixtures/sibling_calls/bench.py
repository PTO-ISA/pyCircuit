# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac
from checks.clients import Relay, Writer
from checks.counter import Counter, Ledger


@ac.system
def ExerciseSiblings():  # noqa: N802
    phase: ac.u4 = 0
    counter = Counter()
    held = Counter()
    ledger = Ledger()
    Writer(counter, ledger, phase != 1, phase == 0)
    Relay(counter, ledger)

    @ac.rule
    def advance() -> None:
        count = counter.old()
        balance, marker = ledger.read()
        ac.log("info", "sibling old", count, balance, marker, held.old())
        assert phase < 4, "deliberate sibling system limit"
        phase = phase + 1
