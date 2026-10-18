"""Stateless forwarding of four lane packets and four engine results."""

import pycircuit as ac


@ac.struct
class Channel:
    valid: ac.u1
    data: ac.bits[128]


@ac.struct
class LaneInput:
    valid: ac.u1
    data: ac.bits[128]
    control: ac.u5


@ac.struct
class EngineResult:
    valid: ac.u1
    data: ac.bits[128]
    latency: ac.u2
    datapath_valid: ac.u1
    datapath_data: ac.bits[128]


@ac.module
def ForwardLane(packet: LaneInput) -> Channel:  # noqa: N802
    return Channel(valid=packet.valid, data=packet.data)


@ac.module
def ForwardEngine(packet: Channel) -> EngineResult:  # noqa: N802
    return EngineResult(valid=packet.valid, data=packet.data)


@ac.module
def Fastfwd(  # noqa: N802 - hardware module definition
    lane0: LaneInput,
    lane1: LaneInput,  # noqa: N802
    lane2: LaneInput,
    lane3: LaneInput,
    engine0: Channel,
    engine1: Channel,
    engine2: Channel,
    engine3: Channel,
) -> tuple[
    ac.u1,
    Channel,
    Channel,
    Channel,
    Channel,
    EngineResult,
    EngineResult,
    EngineResult,
    EngineResult,
]:
    return (
        0,
        ForwardLane(lane0),
        ForwardLane(lane1),
        ForwardLane(lane2),
        ForwardLane(lane3),
        ForwardEngine(engine0),
        ForwardEngine(engine1),
        ForwardEngine(engine2),
        ForwardEngine(engine3),
    )
