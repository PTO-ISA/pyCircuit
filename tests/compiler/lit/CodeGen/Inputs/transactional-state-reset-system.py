# ruff: noqa: N802 -- Captured System.
"""Finite System schedule resetting all state-machine storage.

Each pair observes committed state at both edges. Cycles 0..4 match the state
machine oracle; reset at cycle 5 still observes old state and rising reset
overrides proposed writes. Cycles 6..9 repeat
cycles 0..3, restoring stage=0, permit=0, completed=0, saved=3, narrow=6,
wide=250. The state-machine runner's --reset-samples option retains the numeric
oracle. Keep this public stimulus instead of editing generated harnesses.
"""

import pycircuit as ac
from state_machine.design import Machine


@ac.system(
    samples=(
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 1),
        (1, 1),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
    )
)
def Top():
    Machine()
