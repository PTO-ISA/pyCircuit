from checks.dut import Accumulator
from pycircuit import log, rule, system


@system
def AtomicFailure():
    left = Accumulator(enable=1)
    right = Accumulator(enable=1)  # noqa: F841 -- Independent rollback probe owner.

    @rule
    def drive_left():
        assert False, "atomic rejection"
        log("info", "failed observation", left)

    drive_left()
