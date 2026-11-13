# ruff: noqa: N802, F823, F841 -- captured System state and static child instances.
"""Connected functional tree: 1 -> 9 -> {8, 10, 17}, with five receivers.

This is a test-only combinational tree. It assigns no hardware link latency,
queue capacity or Calendar schedule. Only the current segment is in flight.
Each six-cycle epoch posts, delivers, repeats, delivers a second identity,
idles and retires. Center 9 lands in epoch 1 and only transits in epoch 2.
"""

import pycircuit as ac
from wse_calendar.forward import ForwardHop, HopInput, HopResult
from wse_calendar.receive import ReceiveEngine, Request, Segment


@ac.module
def Endpoint(
    phase: ac.u4,
    node: ac.u6,
    reached: bool,
    hop: HopResult,
    segment: Segment,
    post: bool,
    retire: bool,
    expected: ac.u32,
):
    request = Request(
        kind=1 if post else 3 if retire else 2 if reached and hop.lands else 0,
        segment=segment,
        capacity=16,
        expected=expected,
        compiled_expected=expected,
    )
    result = ReceiveEngine(request, EARLY_SLOTS=2, SEEN_SLOTS=2)

    @ac.rule
    def observe() -> None:
        ac.log(
            "info",
            "multinode_result",
            phase,
            node,
            reached,
            hop.egress,
            hop.lands,
            hop.fault,
            result.accepted,
            result.status,
            result.fault,
            result.active,
            result.counted,
            result.complete,
            result.pending,
        )


# ndf: implements=WSEMIG-MULTINODE-001
@ac.system
def MultiNodeSystem():
    phase: ac.u4 = 0
    post = phase == 0 or phase == 6
    retire = phase == 5 or phase == 11
    send = phase == 1 or phase == 2 or phase == 3 or phase == 7 or phase == 8 or phase == 9
    route = 0xC003F0008 if phase < 6 else 0xC003B0008
    segment = Segment(
        opcode=1,
        epoch=1 if phase < 6 else 2,
        address=104 if phase == 3 or phase == 9 else 100,
        nbytes=4,
        source=1,
        seq=1 if phase == 3 or phase == 9 else 0,
        committed=True,
        self_sourced=False,
    )
    source_hop = ForwardHop(HopInput(route=route, node=1, has_ingress=False, ingress=0))
    center_reached = send and not source_hop.fault and source_hop.egress[9:10] != 0
    center_hop = ForwardHop(HopInput(route=route, node=9, has_ingress=True, ingress=1))
    west_reached = center_reached and not center_hop.fault and center_hop.egress[8:9] != 0
    east_reached = center_reached and not center_hop.fault and center_hop.egress[10:11] != 0
    south_reached = center_reached and not center_hop.fault and center_hop.egress[17:18] != 0
    west_hop = ForwardHop(HopInput(route=route, node=8, has_ingress=True, ingress=9))
    east_hop = ForwardHop(HopInput(route=route, node=10, has_ingress=True, ingress=9))
    south_hop = ForwardHop(HopInput(route=route, node=17, has_ingress=True, ingress=9))
    source = Endpoint(phase, 1, send, source_hop, segment, post, retire, 0)
    center = Endpoint(
        phase, 9, center_reached, center_hop, segment, post, retire, 8 if phase < 6 else 0
    )
    west = Endpoint(phase, 8, west_reached, west_hop, segment, post, retire, 8)
    east = Endpoint(phase, 10, east_reached, east_hop, segment, post, retire, 8)
    south = Endpoint(phase, 17, south_reached, south_hop, segment, post, retire, 8)

    @ac.rule
    def advance() -> None:
        phase = phase + 1
