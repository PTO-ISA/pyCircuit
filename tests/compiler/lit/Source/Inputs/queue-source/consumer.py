# ruff: noqa: I001, N802 -- imported hardware names retain their published spelling.
import pycircuit as ac
from q4_queue.provider import LocalTwin as Pair, Pair13
from q4_queue.provider import LocalNested, PairPacket, Packet


# These parents own no storage or queue. The imported child alone establishes
# the hidden domain, and the provider source is absent during this compilation.
@ac.module
def ScalarParent(valid: ac.u1, data: ac.u13, take: ac.u1) -> Pair13:
    return Pair(valid, data=data, take=take)


@ac.module
def NestedParent(valid: ac.u1, data: Packet, take: ac.u1) -> PairPacket:
    return LocalNested(valid=valid, data=data, take=take)
