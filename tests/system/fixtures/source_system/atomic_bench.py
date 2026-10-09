from checks.dut import Accumulator
from pycircuit import log, rule, system


@system
def AtomicFailure():
    left = Accumulator()
    right = Accumulator()

    @rule
    def drive_left():
        left(enable=1)
        assert False, "atomic rejection"
        log("info", "failed observation", left.value)

    @rule
    def drive_right():
        right(enable=1)

    drive_left()
    drive_right()
