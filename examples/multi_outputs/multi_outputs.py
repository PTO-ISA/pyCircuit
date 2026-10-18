"""Scalar outputs for independent consumers; a Struct for one shared interface."""

import pycircuit as ac


@ac.struct
class Decoded:
    tag: ac.u4
    word: ac.u8
    valid: ac.u1


@ac.module
def Decode(word: ac.u8, enable: ac.u1) -> tuple[ac.u4, ac.u8, ac.u1]:  # noqa: N802
    return word[:4], word, enable


@ac.module
def HeaderConsumer(tag: ac.u4) -> ac.u4:  # noqa: N802
    return tag + 1


@ac.module
def PayloadConsumer(word: ac.u8) -> ac.u8:  # noqa: N802
    return word ^ 255


@ac.module
def MultiOutputs(  # noqa: N802
    word: ac.u8, enable: ac.u1
) -> tuple[ac.u4, ac.u8, ac.u1]:
    tag, payload, valid = Decode(word, enable)
    tag = HeaderConsumer(tag)
    data = PayloadConsumer(payload)
    return tag, data, valid


@ac.module
def DecodePacket(word: ac.u8, enable: ac.u1) -> Decoded:  # noqa: N802
    return Decoded(tag=word[:4], word=word, valid=enable)


@ac.module
def PacketConsumer(packet: Decoded) -> tuple[ac.u4, ac.u8, ac.u1]:  # noqa: N802
    return packet.tag + 1, packet.word ^ 255, packet.valid


@ac.module
def GroupedOutputs(  # noqa: N802
    word: ac.u8, enable: ac.u1
) -> tuple[ac.u4, ac.u8, ac.u1]:
    packet = DecodePacket(word, enable)
    tag, data, valid = PacketConsumer(packet)
    return tag, data, valid
