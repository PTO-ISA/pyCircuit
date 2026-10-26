# ruff: noqa: F823 -- hardware owner writes are captured without Python execution.
import pycircuit as ac
from checks.callable_outputs_provider import Packet, TupleOwner


@ac.rule
def split(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    return value, value + 9


@ac.module
def BehavioralTuple(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    @ac.rule
    def singleton(argument: ac.u8 = value) -> tuple[ac.u8]:
        left, right = split(argument)
        return (right,)

    (saved,) = singleton
    return value, saved


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


@ac.module
def CallableResults() -> tuple[ac.u8, Packet, bool, ac.u8, ac.u8, ac.u8]:
    imported = TupleOwner()
    local = LocalOwner()

    @ac.rule
    def produce(
        delta: ac.u8 = 2, enabled: bool = True
    ) -> tuple[ac.u8, Packet, bool, ac.u8, ac.u8, ac.u8]:
        before, packet, logical = imported.advance(delta, enabled)
        (saved_packet,) = imported.singleton(packet)
        second_before, later_packet, second_flag = imported.advance(3, enabled)
        local_before, local_after = local.advance(1)
        (saved_local,) = local.singleton(local_after)
        assert later_packet.value == second_before + 3
        assert second_flag == logical
        return before, saved_packet, logical, second_before, local_before, saved_local

    return produce


@ac.module
def PhaseResults() -> tuple[ac.u8, ac.u8]:
    phase: ac.u8 = 4

    @ac.rule(guard=lambda owned, increment: True)
    def advance_phase(
        owned: ac.u8 = phase, increment: ac.u8 = 2
    ) -> tuple[ac.u8, ac.u8]:
        before = owned
        owned = owned + increment
        return before, owned

    return advance_phase


@ac.system
def ExerciseCallableOutputs():
    before, packet, logical, second_before, local_before, local_after = (
        CallableResults()
    )
    phase_before, phase_after = PhaseResults()
    behavioral_left, behavioral_right = BehavioralTuple(17)

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
