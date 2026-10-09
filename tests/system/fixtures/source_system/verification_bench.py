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
        left()
        log("info", "left", left.value)

    @rule
    def drive_right():
        right()
        log("info", "right", right.value)

    drive_left()
    drive_right()


@rule
def unrelated_advance(count):
    count = count + 1
    assert count < 9, "unrelated limit"
    log("info", "unrelated", count)


@system
def UnrelatedCheckedRoot():
    count: u8 = 0
    unrelated_advance(count)
