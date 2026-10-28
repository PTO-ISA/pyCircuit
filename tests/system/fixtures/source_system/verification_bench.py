from checks.verification_dut import CheckedChild
from pycircuit import log, rule, system, u8


@rule
def unchecked_advance(count):
    count = count + 1
    log("info", "count", count)


@system
def UncheckedRoot():
    count: u8 = 0
    unchecked_advance(count)


@system
def TwoCheckedChildren():
    left = CheckedChild()
    right = CheckedChild()

    @rule
    def drive_left():
        log("info", "left", left)

    @rule
    def drive_right():
        log("info", "right", right)


@rule
def unrelated_advance(count):
    count = count + 1
    assert count < 9, "unrelated limit"
    log("info", "unrelated", count)


@system
def UnrelatedCheckedRoot():
    count: u8 = 0
    unrelated_advance(count)
