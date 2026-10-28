# ruff: noqa: F823, F841 -- captured module-owned state updates.
import pycircuit as ac


@ac.system
def ExerciseEhrOrder():
    count: ac.u8 = 5
    phase: ac.u3 = 0
    old_count = count

    @ac.rule(guard=lambda: phase != 1)
    def first() -> None:
        count = count + 1

    @ac.rule(after=(first,), guard=lambda: count == 6)
    def second() -> None:
        count = count * 2

    @ac.rule(after=(first,))
    def middle() -> None:
        ac.log("info", "ehr middle", old_count, count)

    @ac.rule(after=(second,))
    def final() -> None:
        ac.log("info", "ehr final", old_count, count)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
