# ruff: noqa: F823 -- hardware owner writes are captured without Python execution.
import pycircuit as ac
from checks.callable_outputs_provider import Packet, TupleOwner


@ac.rule
def split(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    return value, value + 9


@ac.module
def BehavioralTuple(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    @ac.rule
    def singleton(argument: ac.u8) -> tuple[ac.u8]:
        return (argument,)

    left, right = split(value)
    (saved,) = singleton(right)
    return left, saved


@ac.module
def LocalOwner():
    count: ac.u8 = 11

    @ac.method
    def advance(delta: ac.u8) -> tuple[ac.u8, ac.u8]:
        before = count
        count = count + delta
        return before, count

    @ac.method
    def singleton(value: ac.u8) -> tuple[ac.u8]:
        return (value,)


@ac.system
def ExerciseCallableOutputs():
    phase: ac.u8 = 4
    imported = TupleOwner()
    local = LocalOwner()
    behavioral_left, behavioral_right = BehavioralTuple(17)

    @ac.rule
    def produce(
        delta: ac.u8, enabled: bool
    ) -> tuple[ac.u8, Packet, bool, ac.u8, ac.u8, ac.u8]:
        before, packet, logical = imported.advance(delta, enabled)
        (saved_packet,) = imported.singleton(packet)
        second_before, later_packet, second_flag = imported.advance(3, enabled)
        local_before, local_after = local.advance(1)
        (saved_local,) = local.singleton(local_after)
        assert later_packet.value == second_before + 3
        assert second_flag == logical
        return before, saved_packet, logical, second_before, local_before, saved_local

    before, packet, logical, second_before, local_before, local_after = produce(
        enabled=True, delta=2
    )

    @ac.rule(guard=lambda owned, increment: True)
    def advance_phase(owned: ac.u8, increment: ac.u8) -> tuple[ac.u8, ac.u8]:
        before = owned
        owned = owned + increment
        return before, owned

    phase_before, phase_after = advance_phase(increment=2, owned=phase)

    @ac.rule
    def observe() -> None:
        assert packet.value == before + 2
        assert second_before == packet.value
        assert local_after == local_before + 1
        assert logical
        assert phase_after == phase_before + 2
        assert behavioral_left == 17
        assert behavioral_right == 26
        ac.log(
            "info",
            "callable outputs",
            before,
            packet.value,
            logical,
            second_before,
            local_before,
            local_after,
            behavioral_left,
            behavioral_right,
            phase_before,
            phase_after,
        )

    observe()
