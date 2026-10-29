# ruff: noqa: F821, F823, F841 -- Module bindings and state are compiler-owned.
"""Transport a finite stream of known and X/Z packets with backpressure."""

import pycircuit as ac
from packet_case.processor import Processor, Stage
from packet_case.types import Header, Mode, Packet


@ac.module
def PacketStream():  # noqa: N802
    processor: Processor
    sent: ac.u6 = 0
    received: ac.u8 = 0
    cycle: ac.u8 = 0

    @ac.rule(guard=lambda: sent < 4)
    def produce() -> Packet:
        data: ac.bits[80] = (
            ac.bits[80](0x123456789ABCDEF01234)
            if (sent == 0) | (sent == 2)
            else ac.four_state(
                "1010xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01xz01"
            )
        )
        sent = sent + 1
        return Packet(header=Header(opcode=sent, mode=Mode.RAW), data=data)

    incoming = produce()
    head = incoming.peek()

    @ac.autorule
    def observe_head() -> None:
        ac.log("info", "head", head.header.opcode, head.data)

    outgoing, receipts = Stage(incoming, processor)

    @ac.rule(guard=lambda: cycle >= 7)
    def consume(packet: Packet) -> None:
        received = received + 1
        assert packet.header.mode == Mode.DONE
        ac.log("info", "receive", packet.header.opcode, packet.data)

    @ac.rule
    def acknowledge(opcode: ac.u6) -> None:
        ac.log("info", "receipt", opcode)

    consume(outgoing)
    acknowledge(receipts)

    @ac.autorule
    def tick() -> None:
        cycle = cycle + 1
        ac.log("info", "progress", sent, received)
