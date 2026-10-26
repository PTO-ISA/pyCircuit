# ruff: noqa: F823, F841 -- captured module-owned state proposals.
import pycircuit as ac
from checks.hierarchical_table_provider import TableCell


@ac.system
def ExerciseHierarchicalTable():
    selector: ac.u1 = 0
    child = TableCell(selector)
    spare = TableCell(not selector)

    @ac.rule
    def update() -> None:
        if selector:
            child.update(9, 17)

    @ac.rule
    def inspect() -> None:
        rows = child.value()
        other = spare.value()
        ac.log("info", "hierarchical table", rows[0], rows[1], other[0], other[1])

    @ac.rule
    def tick() -> None:
        selector = ~selector
