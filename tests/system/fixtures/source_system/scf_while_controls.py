# ruff: noqa: B007, F823, F841 -- captured loop ordinals and state proposals.
import pycircuit as ac


@ac.rule
def release_depth() -> None:
    # The test repeats this flat clause to exercise nesting-ledger release.
    for index in range(1):
        if False:
            break
    else:
        pass


@ac.system
def ExerciseWhileControls():
    phase: ac.u2 = 0

    @ac.rule
    def advance() -> None:
        nested: ac.u8 = 5
        cursor: ac.u2 = 0
        for index in range(1):
            while cursor < phase:
                cursor = cursor + 1
                nested = nested + 2

        sequential: ac.u8 = 0
        for index in range(2):
            sequential = sequential + 2
        cursor = 0
        while cursor < phase:
            cursor = cursor + 1
            sequential = sequential + 3
        else:
            sequential = sequential + 11

        release_depth()
        ac.log("info", "while controls", nested, sequential)
        phase = phase + 1

    advance()
