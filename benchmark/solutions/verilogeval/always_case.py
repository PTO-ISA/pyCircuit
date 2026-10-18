"""VerilogEval always_case."""

# ruff: noqa: N802 -- benchmark top name is part of the mapping contract.
import pycircuit as ac


@ac.module
def Top(
    sel: ac.u3,
    data0: ac.u4,
    data1: ac.u4,
    data2: ac.u4,
    data3: ac.u4,
    data4: ac.u4,
    data5: ac.u4,
) -> ac.u4:  # noqa: E501, N802
    out = (
        data0
        if sel == 0
        else (
            data1
            if sel == 1
            else (
                data2
                if sel == 2
                else (
                    data3
                    if sel == 3
                    else data4 if sel == 4 else data5 if sel == 5 else 0
                )
            )
        )
    )
    return out
