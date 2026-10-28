# ruff: noqa: F823, F841 -- captured state proposals.
import pycircuit as ac
from checks.output_validity_provider import CapturedPayload, GuardedOutputs


@ac.system
def ExerciseCapturedValidity():
    phase: ac.u8 = 0
    sent: ac.u8 = 7
    packet, extra = GuardedOutputs(phase, phase != 1)
    unused_packet, spare = GuardedOutputs(phase + 7, phase == 1)
    held = CapturedPayload(packet.data, spare)

    @ac.rule
    def invoke() -> None:
        sent = sent + 2
        held.publish()

    @ac.rule
    def inspect() -> None:
        ac.log("info", "captured state", phase, sent)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
