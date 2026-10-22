# ruff: noqa: B007, F821, F823, F841 -- capture-only dead names and state proposals.
import pycircuit as ac


@ac.rule
def finish(value: ac.u8) -> ac.u8:
    for index in range(3):
        value = value + 1
    else:
        value = value + 11
    return value


@ac.system
def ExerciseLoopElseBoundaries():
    phase: ac.u2 = 0
    total: ac.u8 = 5

    @ac.rule
    def advance() -> None:
        saved = total
        index: ac.u8 = 9
        for index in range(0):
            dead = missing_dead_body_name
        else:
            incoming = index
            total = total + incoming
        zero_snapshot = total

        count: ac.u8 = 0
        for ordinal in range(3):
            count = count + 1
            continue
            count = count + 90
        else:
            completed = count + 10

        joined: ac.u8 = 7
        for ordinal in range(3):
            if ordinal == phase:
                break
        else:
            joined = joined + 20

        cursor: ac.u2 = 0
        while cursor < phase:
            cursor = cursor + 1
            if cursor == 2:
                continue
        else:
            finished_cursor: ac.u8 = cursor
            while_result = finished_cursor + 30

        scalar = finish(saved)
        ac.log(
            "info",
            "else boundaries",
            saved,
            incoming,
            zero_snapshot,
            completed,
            joined,
            while_result,
            scalar,
            total,
        )

    @ac.rule
    def tick() -> None:
        phase = phase + 1

    advance()
    tick()
