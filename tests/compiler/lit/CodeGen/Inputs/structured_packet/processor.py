# ruff: noqa: F821, F823, F841 -- Module state is compiler-owned.
"""Update a packet snapshot in the caller's atomic transaction."""

import pycircuit as ac
from packet_case.types import Mode, Packet


@ac.module
def Processor():  # noqa: N802
    accepted: ac.u8 = 0
    retained: Packet()

    @ac.method(guard=lambda packet: packet.header.mode == Mode.RAW)
    def process(packet: Packet) -> Packet:
        result = packet
        result.header.opcode += 1
        result.header.mode = Mode.DONE
        retained = result
        accepted = accepted + 1
        ac.log("info", "process", packet.header.opcode, retained.header.opcode)
        return result

    @ac.autorule
    def observe() -> None:
        ac.log(
            "info",
            "processor",
            accepted,
            retained.header.opcode,
            retained.header.mode == Mode.DONE,
            retained.data,
        )


@ac.module
def Stage(source: Packet, processor: ac.module) -> tuple[Packet, ac.u6]:  # noqa: N802
    @ac.rule
    def transform(packet: Packet) -> tuple[Packet, ac.u6]:
        result: Packet = processor.process(packet)
        return result, packet.header.opcode

    result, receipt = transform(source)
    return result, receipt
