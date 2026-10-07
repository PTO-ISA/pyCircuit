# ruff: noqa: N802, F841, F821 -- hardware module names and unused queue outputs are deliberate.
import pycircuit as ac


@ac.struct
class Header:
    tag: ac.u5 = 17
    flag: ac.u1 = 1


@ac.struct
class Packet:
    header: Header
    word: ac.u13 = 37


@ac.module
def LatencyTwo(
    valid: ac.u1, data: ac.u13, take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.u13}:
    ready, available, value = ac.queue[ac.u13](valid, data, take, depth=2, latency=2)
    return {"ready": ready, "available": available, "head": value}


@ac.module
def LatencyLocal(
    valid: ac.u1, data: ac.u13, take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.u13}:
    ready, available, value = ac.queue[ac.u13](valid, data, take, depth=4, latency=3)
    return {"ready": ready, "available": available, "head": value}


@ac.module
def LatencyBypass(
    valid: ac.u1, data: ac.u13, take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.u13}:
    ready, available, value = ac.queue[ac.u13](
        valid, data, take, depth=4, latency=3, ready_policy="downstream_pop"
    )
    return {"ready": ready, "available": available, "head": value}


@ac.module
def NestedLatency(
    valid: ac.u1, data: Packet, take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": Packet}:
    ready, available, value = ac.queue[Packet](valid, data, take, depth=4, latency=3)
    return {"ready": ready, "available": available, "head": value}


@ac.module
def HugeLatency(
    valid: ac.u1, data: ac.u13, take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.u13}:
    ready, available, value = ac.queue[ac.u13](
        valid, data, take, depth=2, latency=18446744073709551615
    )
    return {"ready": ready, "available": available, "head": value}


@ac.module
def DeadLatency(
    valid: ac.u1, data: ac.u13, take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.u13}:
    ready, available, value = ac.queue[ac.u13](valid, data, take, depth=1, latency=3)
    return {"ready": valid, "available": take, "head": data}
