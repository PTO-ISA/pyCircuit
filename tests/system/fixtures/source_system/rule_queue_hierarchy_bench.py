# ruff: noqa: F823 -- captured module-owned state proposals.
import pycircuit as ac
from checks.rule_queue_hierarchy_provider import HierarchyRelay


@ac.system
def ExerciseQueueHierarchy():
    cycle: ac.u8 = 0
    left = HierarchyRelay()
    right = HierarchyRelay()

    @ac.rule
    def send() -> None:
        if cycle < 6:
            left.send(cycle + 10)
            right.send(cycle + 30)

    @ac.rule
    def receive_left() -> None:
        selected = left.selected()
        if selected:
            value = left.take()
            ac.log("info", "queue hierarchy parent", cycle, value)

    @ac.rule
    def receive_right() -> None:
        selected = right.selected()
        if selected:
            value = right.take()
            ac.log("info", "queue hierarchy parent", cycle, value)

    @ac.rule
    def tick() -> None:
        cycle = cycle + 1  # noqa: F841 - captured hardware state or child instance.


@ac.system
def UnselectedQueueHierarchy():
    spare = HierarchyRelay()

    @ac.rule
    def send() -> None:
        spare.send(99)
