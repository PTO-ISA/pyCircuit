from history_scalar_parameterized.scalar_parameterized_types import (
    scalar_parameterized_types,
)
from pycircuit import bits, log, rule, system, u8


@rule
def advance(incoming, phase):
    if phase == 0:
        incoming = 1
    elif phase == 1:
        incoming = 65535
    elif phase == 2:
        incoming = 65536
    elif phase == 3:
        incoming = 131071
    elif phase == 4:
        incoming = 87381
    elif phase == 5:
        incoming = 43690
    else:
        incoming = 0  # noqa: F841 - hardware register write
    phase = phase + 1  # noqa: F841 - hardware register write


@system
def ScalarParameterizedTypesSystem():
    phase: u8 = 0
    incoming: bits[17] = 0
    dut = scalar_parameterized_types()

    @rule
    def drive():
        dut(value=incoming)

    @rule
    def inspect():
        assert dut.result == incoming, "17-bit identity"
        log("info", "identity", dut.result)

    drive()
    inspect()
    advance(incoming, phase)
