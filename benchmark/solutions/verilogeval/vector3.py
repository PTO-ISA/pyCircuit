"""VerilogEval vector3."""

# ruff: noqa: N802 -- benchmark top name is part of the mapping contract.
import pycircuit as ac


def concatenate(a: ac.u5, b: ac.u5, c: ac.u5, d: ac.u5, e: ac.u5, f: ac.u5) -> ac.u32:
    one: ac.u1 = 1
    return ac.concat(a, b, c, d, e, f, one, one)


@ac.struct
class Outputs:
    w: ac.u8
    x: ac.u8
    y: ac.u8
    z: ac.u8


@ac.module
def Top(
    a: ac.u5, b: ac.u5, c: ac.u5, d: ac.u5, e: ac.u5, f: ac.u5
) -> Outputs:  # noqa: N802
    combined = concatenate(a, b, c, d, e, f)
    return Outputs(
        w=combined[24:32],
        x=combined[16:24],
        y=combined[8:16],
        z=combined[:8],
    )
