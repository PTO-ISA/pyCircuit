# ruff: noqa: B011, F823, F841 -- captured state updates and hardware assertions.
import pycircuit as ac
from checks.output_validity_provider import GuardedOutputs, ValiditySink


@ac.system
def ExerciseOutputValidity():
    phase: ac.u8 = 0
    seen: ac.u8 = 0
    checked_count: ac.u8 = 0
    packet, extra = GuardedOutputs(phase, phase != 1)
    sink = ValiditySink()

    @ac.rule
    def consume() -> None:
        sink.put(packet, extra)

    @ac.rule
    def observe() -> None:
        ac.log("info", "validity destination", sink.read(), phase, seen, checked_count)

    @ac.rule
    def publish() -> None:
        seen = seen + 1
        ac.log("info", "validity output", packet.data, extra)

    @ac.rule
    def check() -> None:
        checked_count = checked_count + 1
        assert packet.data == phase + 1

    @ac.rule
    def selected() -> None:
        if phase == 1:
            ac.log("info", "validity constant", 99)
        else:
            ac.log("info", "validity selected", packet.data)

    @ac.rule
    def tick() -> None:
        phase = phase + 1

    consume()
    observe()
    publish()
    check()
    selected()
    tick()
