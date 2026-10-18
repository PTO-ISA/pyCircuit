"""VerilogEval mux9to1v."""

# ruff: noqa: N802 -- benchmark top name is part of the mapping contract.
import pycircuit as ac


@ac.module
def Top(
    a: ac.u16,
    b: ac.u16,
    c: ac.u16,
    d: ac.u16,
    e: ac.u16,
    f: ac.u16,
    g: ac.u16,
    h: ac.u16,
    i: ac.u16,
    sel: ac.u4,
) -> ac.u16:  # noqa: E501, N802
    out = (
        a
        if sel == 0
        else (
            b
            if sel == 1
            else (
                c
                if sel == 2
                else (
                    d
                    if sel == 3
                    else (
                        e
                        if sel == 4
                        else (
                            f
                            if sel == 5
                            else (
                                g
                                if sel == 6
                                else h if sel == 7 else i if sel == 8 else 0xFFFF
                            )
                        )
                    )
                )
            )
        )
    )
    return out
