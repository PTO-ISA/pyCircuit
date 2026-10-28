# ruff: noqa: F821, F823 -- Module bindings and state are compiler-owned.
"""A delayed consumer backpressures packet processing and its retained state."""

import pycircuit as ac
from example_structured_packet.processor import Processor, Stage
from example_structured_packet.types import Header, Mode, Packet


@ac.system
def Top():  # noqa: N802
    processor: Processor
    sent: ac.u6 = 0
    received: ac.u8 = 0
    cycle: ac.u8 = 0

    @ac.rule(guard=lambda: sent < 4)
    def produce() -> Packet:
        sent = sent + 1
        return Packet(
            header=Header(opcode=sent, mode=Mode.RAW),
            data=ac.bits[80](0x123456789ABCDEF01234),
        )

    incoming = produce()
    head = incoming.peek()

    @ac.autorule
    def observe_head() -> None:
        ac.log("info", "head", head.header.opcode, head.data)

    outgoing = Stage(incoming, processor)

    @ac.rule(guard=lambda: cycle >= 10)
    def consume(packet: Packet) -> None:
        received = received + 1
        assert packet.header.mode == Mode.DONE
        ac.log("info", "receive", packet.header.opcode, packet.data)

    consume(outgoing)

    @ac.autorule
    def tick() -> None:
        cycle = cycle + 1
        ac.log("info", "progress", sent, received)
