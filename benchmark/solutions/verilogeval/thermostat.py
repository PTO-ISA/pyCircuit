"""VerilogEval thermostat."""

# ruff: noqa: N802 -- benchmark top name is part of the mapping contract.
import pycircuit as ac


@ac.struct
class Outputs:
    heater: ac.u1
    aircon: ac.u1
    fan: ac.u1


@ac.module
def Top(
    mode: ac.u1, too_cold: ac.u1, too_hot: ac.u1, fan_on: ac.u1
) -> Outputs:  # noqa: N802
    heater = mode & too_cold
    aircon = ~mode & too_hot
    return Outputs(heater=heater, aircon=aircon, fan=fan_on | heater | aircon)
