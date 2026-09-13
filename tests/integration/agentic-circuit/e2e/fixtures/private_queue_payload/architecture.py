import agentic_circuit as ac


@ac.struct
class Packet:
    tag: ac.u8
    payload: ac.array[16, ac.u64]
    valid: bool


@ac.struct
class Result:
    tag: ac.u8
    valid: bool


@ac.rule
def project(packet: Packet) -> Result:
    return Result(tag=packet.tag, valid=packet.valid)


@ac.system
def private_queue_payload(packet: Packet) -> Result:
    buffered = packet.apply(lambda item: item, depth=2, latency=3)
    projected = project(buffered)
    return projected
