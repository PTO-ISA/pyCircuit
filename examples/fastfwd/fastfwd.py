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


@ac.struct
class FastResult:
    backpressure: ac.u1
    lane0: Channel
    lane1: Channel
    lane2: Channel
    lane3: Channel
    engine0: EngineResult
    engine1: EngineResult
    engine2: EngineResult
    engine3: EngineResult


@ac.module
def ForwardLane(packet: LaneInput) -> Channel:  # noqa: N802
    return Channel(valid=packet.valid, data=packet.data)


@ac.module
def ForwardEngine(packet: Channel) -> EngineResult:  # noqa: N802
    return EngineResult(valid=packet.valid, data=packet.data)


@ac.module
def Fastfwd(lane0: LaneInput, lane1: LaneInput,  # noqa: N802
            lane2: LaneInput, lane3: LaneInput,
            engine0: Channel, engine1: Channel,
            engine2: Channel, engine3: Channel) -> FastResult:
    return FastResult(
        lane0=ForwardLane(lane0), lane1=ForwardLane(lane1),
        lane2=ForwardLane(lane2), lane3=ForwardLane(lane3),
        engine0=ForwardEngine(engine0), engine1=ForwardEngine(engine1),
        engine2=ForwardEngine(engine2), engine3=ForwardEngine(engine3),
    )
