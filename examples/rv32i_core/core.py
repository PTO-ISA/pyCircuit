# ruff: noqa: F823, F841, N802 -- captured hardware declarations and state.
from rv32i_core.types import initial_machine
import pycircuit as ac
from rv32i_core.types import BusInputs, Machine, Pins
from rv32i_core.writeback import Writeback
from rv32i_core.memory import Memory
from rv32i_core.execute import ExecuteLogic
from rv32i_core.frontend import Frontend, Finalize


# ndf: implements=RV32-SPEC-STATE-001,RV32-SPEC-VIEWS-001
@ac.module(execution={"profile": "clocked_fixedpoint_v1", "state_output": 1})
def Core(bus: BusInputs) -> tuple[Pins, Machine]:
    state: Machine = initial_machine()
    s1, p1, wb = Writeback(state, bus)
    s2, p2, mem, mc = Memory(state, s1, p1, wb, bus)
    s3, p3, ec = ExecuteLogic(state, s2, p2, wb, mem, mc)
    s4, p4 = Frontend(state, s3, p3, ec, bus)

    following, pins = Finalize(state, s1, p1, s4, p4, wb.terminal)

    # ndf: implements=RV32-SPEC-STATE-001,RV32-SPEC-COMMIT-001
    @ac.rule
    def step() -> Pins:
        state = following
        return pins

    return step, state
