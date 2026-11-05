# ruff: noqa: N802, F823, F841 -- pyCircuit declarations and captured state.
"""Persistent 8-bit epoch bank and resident CalReg installation checks.

The epoch width is the accepted test profile. CalReg content stays opaque;
this module implements installation/lookup admission, not a timeslot schedule.
`drained` is an external precondition, not an inferred network measurement.
An invalid request reports fault and leaves persistent state unchanged.
"""

import pycircuit as ac


@ac.struct
class EpochRequest:
    advance: bool
    opcode: ac.u3
    drained: bool


@ac.struct
class EpochResult:
    value: ac.u8
    fault: bool


# ndf: implements=WSEMIG-EPOCH-001
@ac.module
def EpochBank(request: EpochRequest) -> EpochResult:
    counters = ac.table[8, ac.u8](init=0)

    @ac.rule
    def step() -> EpochResult:
        value = counters[request.opcode]
        fault = False
        if request.advance:
            if request.opcode == 0 or request.opcode == 7:
                fault = True
            elif value == 255 and not request.drained:
                fault = True
            else:
                value = 1 if value == 255 else value + 1
                counters[request.opcode] = value
        return EpochResult(value=value, fault=fault)

    return step


@ac.struct
class CalRegRequest:
    # 0 idle, 1 clear bank, 2 install one slot, 3 lookup one slot.
    kind: ac.u2
    opcode: ac.u3
    drained: bool


@ac.struct
class CalRegResult:
    installed: ac.u8
    admitted: bool
    fault: bool


# ndf: implements=WSEMIG-CALREG-001
@ac.module
def CalRegAdmission(request: CalRegRequest) -> CalRegResult:
    installed: ac.u8 = 0

    @ac.rule
    def step() -> CalRegResult:
        bit: ac.u8 = 1
        bit = bit << request.opcode
        exists = (installed & bit) != 0
        fault = False
        if request.kind == 1:
            if request.drained:
                installed = 0
            else:
                fault = True
        elif request.kind != 0:
            if request.opcode == 0 or request.opcode == 7:
                fault = True
            elif request.kind == 2:
                if not request.drained or exists:
                    fault = True
                else:
                    installed = installed | bit
            elif not exists:
                fault = True
        return CalRegResult(
            installed=installed, admitted=request.kind != 0 and not fault, fault=fault
        )

    return step
