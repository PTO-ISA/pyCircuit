from checks.dut import Accumulator
from pycircuit import bits, log, report, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseAccumulator():
    phase: bits[8] = 0
    left = Accumulator()
    right = Accumulator()

    @rule
    def drive_left():
        left(enable=(phase & 1) == 0)
        assert right.value < 4, "deliberate limit"
        log("info", "left", left.value)
        log("info", "right", right.value)
        report("progress", phase)

    @rule
    def drive_right():
        right(enable=1)

    advance(phase)
    drive_left()
    drive_right()
