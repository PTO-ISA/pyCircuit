from pycircuit import system, rule, log, report
from .types import Word, Phase
from .counter import Counter

@system
def TestCounters():
    left_input: Word = 3
    right_input: Word = 10
    left_output: Word = 100
    right_output: Word = 200
    phase: Phase = 0
    left = Counter(left_input, left_output)
    right = Counter(right_input, right_output)

    @rule
    def fixture():
        nonlocal phase
        if phase == 0:
            assert left_output == 100 and right_output == 200, "reset values"
        elif phase == 1:
            assert left_output == 0 and right_output == 0, "first commit"
            log("info", "left", left_output)
            log("info", "right", right_output)
        elif phase == 2:
            assert left_output == 3 and right_output == 10, "second commit"
            log("info", "left", left_output)
            log("info", "right", right_output)
            report("completed", 1)
        if phase < 3:
            phase = phase + 1

    fixture()
