"""VerilogEval m2014_q4c using compiler-owned synchronous reset."""

# ruff: noqa: F841 -- Rule assignments update Module-owned arguments.
import pycircuit as ac


@ac.rule
def remember(saved: ac.u1, incoming: ac.u1) -> None:
    saved = incoming


@ac.module
def Top(incoming: ac.u1) -> ac.u1:  # noqa: N802
    saved: ac.u1 = 0
    remember(saved, incoming)
    return saved
