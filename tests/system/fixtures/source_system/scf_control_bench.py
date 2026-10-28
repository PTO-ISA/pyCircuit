# ruff: noqa: F823, F841 -- captured state proposals.
import pycircuit as ac


@ac.system
def ExerciseScfControl():
    phase: ac.u3 = 0
    total: ac.u8 = 3

    @ac.rule
    def advance() -> None:
        amount: ac.u8 = 0
        last: ac.u3 = 7
        for index in range(6):
            last = index
            if index == phase:
                break
            if index == 1:
                continue
            index_value: ac.u8 = index
            amount = amount + index_value
        else:
            amount = amount + 40
        cursor: ac.u3 = 0
        while cursor < phase:
            cursor = cursor + 1
            if cursor == 2:
                continue
            if cursor == 5:
                break
            cursor_value: ac.u8 = cursor
            amount = amount + cursor_value
        else:
            amount = amount + 80
        before = total
        total = total + amount
        ac.log("info", "scf control", before, amount, last, cursor, total)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
