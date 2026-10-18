"""VerilogEval dff8r using compiler-owned synchronous reset."""

# ruff: noqa: F841 -- Rule assignments update Module-owned arguments.
import pycircuit as ac


@ac.rule
def remember(saved: ac.u8, incoming: ac.u8) -> None:
    saved = incoming


@ac.module
def Top(incoming: ac.u8) -> ac.u8:  # noqa: N802
    saved: ac.u8 = 0
    remember(saved, incoming)
    return saved
