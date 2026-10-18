"""Independent typed module outputs connected to separate consumers."""

import pycircuit as ac


@ac.struct
class Header:
    tag: ac.u4


@ac.struct
class Payload:
    data: ac.u8


@ac.struct
class Decoded:
    header: Header
    payload: Payload
    valid: ac.u1


@ac.module
def Decode(word: ac.u8, enable: ac.u1) -> tuple[Header, Payload, ac.u1]:  # noqa: N802
    header = Header(tag=word[:4])
    payload = Payload(data=word)
    return header, payload, enable


@ac.module
def HeaderConsumer(header: Header) -> tuple[ac.u4]:  # noqa: N802
    return (header.tag + 1,)


@ac.module
def PayloadConsumer(payload: Payload) -> tuple[ac.u8]:  # noqa: N802
    return (payload.data ^ 255,)


@ac.module
def MultiOutputs(  # noqa: N802
    word: ac.u8, enable: ac.u1
) -> tuple[ac.u4, ac.u8, ac.u1]:
    header, payload, valid = Decode(word, enable)
    (tag,) = HeaderConsumer(header)
    (data,) = PayloadConsumer(payload)
    return tag, data, valid


@ac.module
def DecodePacket(word: ac.u8, enable: ac.u1) -> Decoded:  # noqa: N802
    return Decoded(
        header=Header(tag=word[:4]), payload=Payload(data=word), valid=enable
    )


@ac.module
def PacketConsumer(packet: Decoded) -> tuple[ac.u4, ac.u8, ac.u1]:  # noqa: N802
    return packet.header.tag + 1, packet.payload.data ^ 255, packet.valid


@ac.module
def GroupedOutputs(  # noqa: N802
    word: ac.u8, enable: ac.u1
) -> tuple[ac.u4, ac.u8, ac.u1]:
    packet = DecodePacket(word, enable)
    tag, data, valid = PacketConsumer(packet)
    return tag, data, valid
