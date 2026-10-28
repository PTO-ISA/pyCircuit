from checks.dut import Accumulator
from pycircuit import bits, log, report, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseAccumulator():
    phase: bits[8] = 0
    left = Accumulator(enable=(phase & 1) == 0)
    right = Accumulator(enable=1)

    @rule
    def drive_left():
        assert right < 4, "deliberate limit"
        log("info", "left", left)
        log("info", "right", right)
        report("progress", phase)

    advance(phase)
