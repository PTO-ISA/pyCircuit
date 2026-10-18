# ruff: noqa: B011, F823, F841 -- captured proposals and hardware assertions.
import pycircuit as ac
from checks.hierarchical_state_provider import OrderedStateCell, PayloadRelay, StateCell


@ac.system
def ExerciseHierarchicalState():
    selector: ac.u1 = 0
    left = StateCell(selector)
    right = StateCell(not selector)
    relayed = PayloadRelay(selector)
    standalone = PayloadRelay(not selector)

    @ac.rule
    def update_left() -> None:
        if selector:
            before, after = left.update(9)
            assert after == before + 9

    @ac.rule
    def update_right() -> None:
        if not selector:
            before, after = right.update(17)
            assert after == before + 17

    @ac.rule
    def update_relayed() -> None:
        if selector:
            relayed.update(11)

    @ac.rule
    def inspect() -> None:
        ac.log("info", "hierarchical state", left.value(), right.value(), selector)
        packet = relayed.value()
        spare = standalone.value()
        ac.log(
            "info",
            "hierarchical payload",
            packet.data,
            ac.enum_to_bits(packet.tag),
            spare.data,
            ac.enum_to_bits(spare.tag),
        )

    @ac.rule
    def tick() -> None:
        selector = ~selector

    update_left()
    update_right()
    update_relayed()
    inspect()
    tick()


@ac.system
def HierarchicalStateProbe():
    selector: ac.u1 = 0
    left = StateCell(selector)
    right = StateCell(not selector)

    @ac.rule
    def drive() -> None:
        if selector:
            left.update(9)
        else:
            right.update(17)

    @ac.rule
    def inspect() -> None:
        ac.log("info", "hierarchical probe", left.value(), right.value())

    @ac.rule
    def tick() -> None:
        selector = ~selector

    drive()
    inspect()
    tick()


@ac.system
def ExerciseOrderedHierarchicalState():
    child = OrderedStateCell()

    @ac.rule
    def first() -> None:
        before, after = child.update(3)
        ac.log("info", "hierarchical ordered first", before, after)

    @ac.rule(after=(first,))
    def second() -> None:
        before, after = child.update(5)
        ac.log("info", "hierarchical ordered second", before, after)

    @ac.rule
    def inspect() -> None:
        ac.log("info", "hierarchical ordered old q", child.value())

    first()
    second()
    inspect()
