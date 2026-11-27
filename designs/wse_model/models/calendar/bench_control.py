# ruff: noqa: N802, F823, F841 -- captured System state.
"""261 cycles: all 8-bit epochs, rejected wrap, hold, drain and opcode isolation."""

import pycircuit as ac
from wse_calendar.control import CalRegAdmission, CalRegRequest, EpochBank, EpochRequest


@ac.system
def ControlSystem():
    phase: ac.u9 = 0
    epoch = EpochBank(
        EpochRequest(
            advance=phase != 257,
            opcode=2 if phase == 255 or phase == 259 else 0 if phase == 260 else 1,
            drained=phase == 258,
        )
    )
    calreg = CalRegAdmission(
        CalRegRequest(
            kind=(
                3
                if phase == 0 or phase == 2 or phase == 6 or phase == 9
                else (
                    2
                    if phase == 1 or phase == 3 or phase == 7 or phase == 8
                    else 1 if phase == 4 or phase == 5 else 0
                )
            ),
            opcode=7 if phase == 7 else 2 if phase == 8 or phase == 9 else 1,
            drained=phase != 4,
        )
    )

    # ndf: verifies=WSEMIG-CONTROL-VER-001
    @ac.rule
    def check() -> None:
        expected_epoch: ac.u8 = 1
        if phase <= 254:
            expected_epoch = phase[0:8] + 1
        elif phase == 256 or phase == 257:
            expected_epoch = 255
        elif phase == 259:
            expected_epoch = 2
        elif phase == 260:
            expected_epoch = 0
        assert epoch.value == expected_epoch, "epoch persistence and opcode isolation"
        assert epoch.fault == (phase == 256 or phase == 260), "epoch fault"
        expected_installed: ac.u8 = 2 if phase >= 1 and phase <= 4 else 4 if phase >= 8 else 0
        expected_fault = phase == 0 or phase == 3 or phase == 4 or phase == 6 or phase == 7
        assert calreg.installed == expected_installed, "resident installation map"
        assert calreg.fault == expected_fault, "configuration fault"
        assert calreg.admitted == (
            phase == 1 or phase == 2 or phase == 5 or phase == 8 or phase == 9
        ), "configuration admission"
        epoch_fault: ac.u1 = 1 if epoch.fault else 0
        config_fault: ac.u1 = 1 if calreg.fault else 0
        config_admitted: ac.u1 = 1 if calreg.admitted else 0
        ac.log(
            "info",
            "control_checked",
            phase,
            epoch.value,
            calreg.installed,
            epoch_fault,
            config_fault,
            config_admitted,
        )
        phase = phase + 1
