from checks.record_types import Inner, Packet
from pycircuit import log, rule, system, u8


@rule
def construct_and_advance(count) -> Packet:
    result: Packet = Packet(inner=Inner(flag=1, data=count), sequence=3)
    count = count + 1
    log("info", "payload", result.inner.data)
    log("info", "flag", result.inner.flag)
    log("info", "sequence", result.sequence)
    return result


@system
def ConstructPacket():
    count: u8 = 0
    construct_and_advance(count)
