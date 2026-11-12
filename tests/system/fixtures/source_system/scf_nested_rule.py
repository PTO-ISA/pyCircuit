# ruff: noqa: B007, F821, F823, F841 -- capture-only dead names and state proposals.
import pycircuit as ac


@ac.system
def ExerciseNestedRule():
    phase: ac.u2 = 0
    total: ac.u8 = 240

    @ac.rule
    def advance() -> None:
        before = total
        for outer in range(3):
            for middle in range(2):
                for inner in range(2):
                    if inner == 0:
                        continue
                    total = total + 1
                else:
                    total = total + 2
            if outer == phase:
                break
            total = total + 10
        else:
            total = total + 20
        ac.log("info", "nested rule", before, total)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
