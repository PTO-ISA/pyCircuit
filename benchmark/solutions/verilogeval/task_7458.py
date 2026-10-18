"""VerilogEval 7458."""

# ruff: noqa: N802 -- benchmark top name is part of the mapping contract.
import pycircuit as ac


@ac.struct
class Outputs:
    p1y: ac.u1
    p2y: ac.u1


@ac.module
def Top(
    p1a: ac.u1,
    p1b: ac.u1,
    p1c: ac.u1,
    p1d: ac.u1,
    p1e: ac.u1,
    p1f: ac.u1,
    p2a: ac.u1,
    p2b: ac.u1,
    p2c: ac.u1,
    p2d: ac.u1,
) -> Outputs:  # noqa: E501, N802
    return Outputs(
        p1y=(p1a & p1b & p1c) | (p1d & p1e & p1f),
        p2y=(p2a & p2b) | (p2c & p2d),
    )
