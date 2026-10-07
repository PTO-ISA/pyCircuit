# ruff: noqa: F821, F841, I001, N802 -- hardware names and forward source values are intentional.
import pycircuit as ac
from typing import Annotated


@ac.struct
class ScalarResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u13


@ac.struct
class Pair13:
    local: ScalarResult
    bypass: ScalarResult


@ac.struct
class Header:
    tag: ac.u5
    flag: ac.u1


@ac.struct
class Packet:
    header: Header
    word: ac.u13


@ac.struct
class NestedResult:
    ready: ac.u1
    valid: ac.u1
    data: Packet


@ac.struct
class PairPacket:
    local: NestedResult
    bypass: NestedResult


@ac.struct
class One:
    value: ac.u1


@ac.struct
class Two:
    value: ac.u2


@ac.module
def BooleanEcho(value: bool) -> One:
    return One(value=value)


@ac.module
def IntegerEcho(value: Annotated[int, range(1 << 1)]) -> One:
    return One(value=value)


@ac.module
def FixedEcho(value: ac.u1) -> One:
    return One(value=value)


@ac.module
def Fixed2Echo(value: ac.u2) -> Two:
    return Two(value=value)


# Mapping-return modules remain structurally valid without the optional
# typed-result source-call metadata bundle.
@ac.module
def LegacyEcho(value: ac.u1) -> {"out": One}:
    return {"out": One(value=value)}


@ac.module
def BooleanLocal(value: bool) -> One:
    return BooleanEcho(value)


@ac.module
def IntegerLocal(value: Annotated[int, range(1 << 1)]) -> One:
    return IntegerEcho(value)


@ac.module
def FixedLocal(value: ac.u1) -> One:
    return FixedEcho(value)


@ac.module
def BooleanToFixedLocal(value: bool) -> One:
    converted = FixedEcho(value)
    original = BooleanEcho(value)
    return original


@ac.module
def IntegerToFixedLocal(value: Annotated[int, range(1 << 1)]) -> One:
    converted = FixedEcho(value)
    original = IntegerEcho(value)
    return original


@ac.module
def FixedWidenLocal(value: ac.u1) -> One:
    widened = Fixed2Echo(value)
    original = FixedEcho(value)
    return original


@ac.module
def ScalarDefault(valid: ac.u1, data: ac.u13, take: ac.u1) -> ScalarResult:
    ready, available, value = ac.queue[ac.u13](valid, data, take)
    return ScalarResult(ready=ready, valid=available, data=value)


@ac.module
def ScalarBypass(valid: ac.u1, data: ac.u13, take: ac.u1) -> ScalarResult:
    ready, available, value = ac.queue[ac.u13](
        in_valid=valid,
        in_data=data,
        out_ready=take,
        depth=2,
        ready_policy="downstream_pop",
        latency=1,
    )
    return ScalarResult(ready=ready, valid=available, data=value)


@ac.module
def NestedDefault(valid: ac.u1, data: Packet, take: ac.u1) -> NestedResult:
    ready, available, value = ac.queue[Packet](valid, data, take, depth=3)
    return NestedResult(ready=ready, valid=available, data=value)


@ac.module
def NestedBypass(valid: ac.u1, data: Packet, take: ac.u1) -> NestedResult:
    ready, available, value = ac.queue[Packet](
        valid, data, take, depth=3, ready_policy="downstream_pop"
    )
    return NestedResult(ready=ready, valid=available, data=value)


@ac.module
def LocalTwin(valid: ac.u1, data: ac.u13, take: ac.u1) -> Pair13:
    local = ScalarDefault(valid, data, take)
    bypass = ScalarBypass(valid, data, take)
    return Pair13(local=local, bypass=bypass)


@ac.module
def LocalNested(valid: ac.u1, data: Packet, take: ac.u1) -> PairPacket:
    local = NestedDefault(valid, data, take)
    bypass = NestedBypass(valid, data, take)
    return PairPacket(local=local, bypass=bypass)


@ac.module
def Wide65(
    valid: ac.u1, data: ac.bits[65], take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.bits[65]}:
    ready, available, value = ac.queue[ac.bits[65]](valid, data, take)
    return {"ready": ready, "available": available, "head": value}


@ac.module
def Wide130(
    valid: ac.u1, data: ac.bits[130], take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.bits[130]}:
    ready, available, value = ac.queue[ac.bits[130]](valid, data, take, depth=5)
    return {"ready": ready, "available": available, "head": value}


@ac.module
def TablePayload(
    valid: ac.u1, data: ac.table[3, ac.u13], take: ac.u1
) -> {"ready": ac.u1, "available": ac.u1, "head": ac.table[3, ac.u13]}:
    ready, available, value = ac.queue[ac.table[3, ac.u13]](valid, data, take, depth=3)
    return {"ready": ready, "available": available, "head": value}
