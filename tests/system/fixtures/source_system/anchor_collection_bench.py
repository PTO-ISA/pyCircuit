# ruff: noqa: F823, F841 -- captured state proposals.
import pycircuit as ac


@ac.system
def ExerciseAnchorCollection():
    # The admitted persistent-Table form stays compact logical state in the
    # anchor and refines to a DFFE Collection, as in examples/table_rule.
    rows = ac.table[4, ac.u8](init=2)
    position: ac.u2 = 0

    @ac.rule
    def update() -> None:
        old = rows[position]
        rows[position] = old + 3
        ac.log(
            "info",
            "anchor collection",
            position,
            old,
            rows[0],
            rows[1],
            rows[2],
            rows[3],
        )
        position = position + 1
