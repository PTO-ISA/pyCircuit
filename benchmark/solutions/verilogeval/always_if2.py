"""VerilogEval always_if2."""

# ruff: noqa: N802 -- benchmark top name is part of the mapping contract.
import pycircuit as ac


@ac.struct
class Outputs:
    shut_off_computer: ac.u1
    keep_driving: ac.u1


@ac.module
def Top(
    cpu_overheated: ac.u1, arrived: ac.u1, gas_tank_empty: ac.u1
) -> Outputs:  # noqa: N802
    return Outputs(
        shut_off_computer=cpu_overheated,
        keep_driving=(~arrived) & (~gas_tank_empty),
    )
