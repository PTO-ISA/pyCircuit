from pycircuit import bits, log, module, rule, system, u8


@module
def DifferentIdentity(
    value: bits[width], *, width: int = 32
) -> {"result": bits[width]}:
    return {"result": value}


@rule
def advance(incoming, phase):
    if phase == 0:
        incoming = 1
    elif phase == 1:
        incoming = 65535
    elif phase == 2:
        incoming = 65536
    elif phase == 3:
        incoming = 4294967295
    elif phase == 4:
        incoming = 2863311530
    elif phase == 5:
        incoming = 1431655765
    else:
        incoming = 0  # noqa: F841 - hardware register write
    phase = phase + 1  # noqa: F841 - hardware register write


@system
def DifferentDefaultSystem():
    phase: u8 = 0
    incoming: bits[32] = 0
    dut = DifferentIdentity()

    @rule
    def drive():
        dut(value=incoming)

    @rule
    def inspect():
        assert dut.result == incoming, "different32-bit identity"
        log("info", "identity", dut.result)

    drive()
    inspect()
    advance(incoming, phase)
