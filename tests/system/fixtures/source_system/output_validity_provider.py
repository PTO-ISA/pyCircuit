# ruff: noqa: F823, F841 -- captured module-owned state updates.
import pycircuit as ac


@ac.struct
class Packet:
    data: ac.u8


@ac.module
def GuardedOutputs(word: ac.u8, enabled: ac.u1) -> tuple[Packet, ac.u8]:
    @ac.rule(guard=lambda: enabled)
    def produce() -> tuple[Packet, ac.u8]:
        return Packet(data=word + 1), word + 3

    packet, extra = produce()
    return packet, extra


@ac.module
def ValiditySink():
    saved: ac.u8 = 0

    @ac.rule
    def put(packet: Packet, extra: ac.u8) -> None:
        saved = packet.data + extra

    @ac.rule
    def read() -> ac.u8:
        return saved


@ac.module
def CapturedPayload(word: ac.u8, spare: ac.u8):
    @ac.rule
    def publish() -> None:
        ac.log("info", "captured payload", word)
