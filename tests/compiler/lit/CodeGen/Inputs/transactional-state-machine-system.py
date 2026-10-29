# ruff: noqa: N802 -- Captured System.
import pycircuit as ac
from state_machine.design import Machine


@ac.system
def Top():
    Machine()
