# ruff: noqa: N802, F823, F841 -- captured System state.
"""Thirteen-cycle node integration with two receive rounds on the 40-node carrier.

The route is source 1 to target 9. Installation uses an explicit drained test
precondition; CalReg contents remain opaque. The schedule assigns no hardware
latency or network-drain algorithm. Epoch, decode, forwarding and admission
outputs are connected to the actual receive request, with no expected-result
oracle in this source.
"""

import pycircuit as ac
from wse_calendar.control import CalRegAdmission, CalRegRequest, EpochBank, EpochRequest
from wse_calendar.forward import ForwardHop, HopInput
from wse_calendar.receive import ReceiveEngine, Request, Segment
from wse_calendar.route import DecodeEntry, RouteRegisters


# ndf: implements=WSEMIG-NODE-001
@ac.system
def NodeSystem():
    cycle: ac.u4 = 0
    post = cycle == 2 or cycle == 7
    retire = cycle == 6 or cycle == 11
    send_attempt = (
        cycle == 0
        or cycle == 3
        or cycle == 4
        or cycle == 5
        or cycle == 8
        or cycle == 9
        or cycle == 10
    )
    epoch = EpochBank(EpochRequest(advance=post, opcode=1, drained=False))
    config = CalRegAdmission(
        CalRegRequest(
            kind=2 if cycle == 1 else 3 if send_attempt or cycle == 12 else 0,
            opcode=1,
            drained=cycle == 1,
        )
    )
    # Shared externally validated tree: P at 1 and 9, L only at 9.
    # Metadata belongs to receiving member 9, not source 1's local send row:
    # one landing, member flag, and two four-byte identities expected here.
    entry = RouteRegisters(lo=0xC0008, hi=0x0000000801010000)
    decoded = DecodeEntry(entry, NODE=9)
    route = ac.concat(entry.hi[0:16], entry.lo)
    source_hop = ForwardHop(HopInput(route=route, node=1, has_ingress=False, ingress=0))
    target_hop = ForwardHop(HopInput(route=route, node=9, has_ingress=True, ingress=1))
    reached = (
        send_attempt
        and config.admitted
        and not epoch.fault
        and not decoded.fault
        and not source_hop.fault
        and source_hop.egress[9:10] != 0
    )
    segment = Segment(
        opcode=1,
        epoch=epoch.value,
        address=104 if cycle == 5 or cycle == 10 else 100,
        nbytes=4,
        source=1,
        seq=1 if cycle == 5 or cycle == 10 else 0,
        committed=True,
        self_sourced=False,
    )
    request = Request(
        kind=1 if post else 3 if retire else 2 if reached and target_hop.lands else 0,
        segment=segment,
        capacity=16,
        expected=decoded.expected_bytes,
        compiled_expected=decoded.expected_bytes,
    )
    result = ReceiveEngine(request, EARLY_SLOTS=2, SEEN_SLOTS=2)

    @ac.rule
    def observe() -> None:
        ac.log(
            "info",
            "node_result",
            cycle,
            epoch.value,
            epoch.fault,
            config.installed,
            config.admitted,
            config.fault,
            decoded.pair,
            decoded.land_count,
            decoded.flags,
            decoded.expected_bytes,
            decoded.fault,
            send_attempt,
            reached,
            source_hop.egress,
            target_hop.egress,
            target_hop.lands,
            source_hop.fault,
            target_hop.fault,
            result.accepted,
            result.status,
            result.fault,
            result.active,
            result.counted,
            result.complete,
            result.pending,
            segment.epoch,
            segment.source,
            segment.seq,
            segment.address,
            segment.nbytes,
        )
        cycle = cycle + 1
