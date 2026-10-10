"""Closed failure system for values produced by a bounded loop."""

# ruff: noqa: B011, N802 -- the failing hardware assertion is intentional.

from pycircuit import rule, system, u8


@rule
def advance(epoch):
    epoch = epoch + 1


@rule
def reject_loop(epoch: u8):
    total: u8 = 0
    for index in range(3):
        fixed_index: u8 = index
        total = total + fixed_index
    assert (total + epoch) == 0, "bounded_loop_outside_assert"


@system
def LoopAssertionSystem():  # noqa: N802
    epoch: u8 = 0
    advance(epoch)
    reject_loop(epoch)
