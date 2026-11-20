# ruff: noqa: N802, N803, F823, F841 -- pyCircuit modules and captured state.
"""Completion-driven control for the existing serialized 32-member carrier.

Each member owns its CalReg admission, epoch bank and receive account. Stage
changes consume their actual results. The transport still has one outstanding
multicast segment, delivered synchronously; retrying a blocked landing retains
its identity and relies on the existing receive de-duplication semantics.
"""

import pycircuit as ac
from wse_calendar.allgather import Transfer, hop_rows, line_input, path_reached, route_rows
from wse_calendar.control import CalRegAdmission, CalRegRequest, EpochBank, EpochRequest
from wse_calendar.forward import ForwardHop, HopResult
from wse_calendar.receive import ReceiveEngine, Request, Segment
from wse_calendar.route import DecodeEntry, RouteRegisters


@ac.struct
class CarrierInput:
    cycle: ac.u10
    post_permit: ac.bits[32]
    landing_permit: ac.bits[32]


@ac.struct
class Command:
    # 0 install, 1 epoch, 2 post, 3 send, 4 join, 5 retire, 6 done, 7 fault.
    stage: ac.u3
    transfer: Transfer
    post_permit: ac.bits[32]
    landing_permit: ac.bits[32]


@ac.struct
class MemberFeedback:
    epoch: ac.u8
    configured: bool
    posted: bool
    sent: bool
    complete: bool
    drained: bool
    retired: bool
    fault: bool


def local_entry(key: ac.u1, node: ac.u6) -> RouteRegisters:
    rows = route_rows()
    index: ac.u6 = node
    if key == 1:
        index = index + 32
    route = rows[index]
    # Exact FFN fixture metadata: isMember/isRoot, non-self landing count and expVal.
    high: ac.u64 = 0x0000900003030000 if key == 1 else 0x00002A0003070000
    route_high: ac.u64 = route[64:80]
    high = high | route_high
    return RouteRegisters(lo=route[0:64], hi=high)


def controlled_transfer(
    cycle: ac.u10, stage: ac.u3, key: ac.u1, cursor: ac.u8, epoch: ac.u8
) -> Transfer:
    sending = stage == 3
    source: ac.u6 = cursor[3:8] if sending else 0
    row: ac.u3 = cursor[0:3] if sending else 0
    entry = local_entry(key, source)
    column = key == 1
    rank = source[3:6] if column else source[0:3]
    nbytes: ac.u32 = 1536 if column else 192
    stride_bytes: ac.u64 = 6144 if column else 1536
    rank_wide: ac.u64 = rank
    row_wide: ac.u64 = row
    bytes_wide: ac.u64 = nbytes
    seq: ac.u16 = row
    if column:
        seq = seq + 8
    return Transfer(
        cycle=cycle,
        key_id=key,
        source=source,
        row=row,
        seq=seq,
        epoch=epoch,
        route=ac.concat(entry.hi[0:16], entry.lo),
        address=4096 + row_wide * stride_bytes + rank_wide * bytes_wide,
        nbytes=nbytes,
        capacity=49152 if column else 12288,
        expected=36864 if column else 10752,
        post=stage == 2,
        retire=stage == 5,
        send=sending,
        base=source & 7 if column else source & 56,
        stride=8 if column else 1,
        source_rank=rank,
    )


# ndf: implements=WSEMIG-CLOSEDLOOP-MEMBER-001
@ac.module
def ControlledMember(
    command: Command, hops: ac.table[8, HopResult], *, NODE: int
) -> MemberFeedback:
    configured: bool = False
    posted: bool = False
    node: ac.u6 = NODE
    stage = command.stage
    transfer = command.transfer
    config = CalRegAdmission(
        CalRegRequest(
            kind=2 if stage == 0 and not configured else 3 if stage != 7 else 0,
            opcode=1,
            # Reset starts empty; no send is possible before installation completes.
            drained=stage == 0,
        )
    )
    # The controller enters epoch only after all accounts retire with no pending
    # entries; the serialized carrier has no separate in-flight link storage.
    epoch = EpochBank(EpochRequest(advance=stage == 1, opcode=1, drained=not posted))
    decoded = DecodeEntry(local_entry(transfer.key_id, node), NODE=NODE)
    rank = node[3:6] if transfer.key_id == 1 else node[0:3]
    same_group = (
        node[0:3] == transfer.source[0:3]
        if transfer.key_id == 1
        else node[3:6] == transfer.source[3:6]
    )
    routing = transfer.send and same_group
    hop = hops[rank]
    reached = routing and path_reached(hops, transfer, rank)
    target = routing and node != transfer.source
    admitted = config.admitted and not config.fault and not decoded.fault and not epoch.fault
    post_allowed = (command.post_permit >> node) & 1 != 0
    landing_allowed = (command.landing_permit >> node) & 1 != 0
    post = transfer.post and not posted and admitted and post_allowed
    deliver = reached and hop.lands and admitted and landing_allowed
    segment = Segment(
        opcode=1,
        epoch=epoch.value if post else transfer.epoch,
        address=transfer.address,
        nbytes=transfer.nbytes,
        source=transfer.source,
        seq=transfer.seq,
        committed=True,
        self_sourced=node == transfer.source,
    )
    request = Request(
        kind=1 if post else 3 if transfer.retire and posted and admitted else 2 if deliver else 0,
        segment=segment,
        capacity=transfer.capacity,
        expected=transfer.expected,
        compiled_expected=decoded.expected_bytes,
    )
    result = ReceiveEngine(request, EARLY_SLOTS=2, SEEN_SLOTS=56)

    @ac.rule
    def observe() -> MemberFeedback:
        if stage == 0 and config.admitted and not config.fault:
            configured = True
        if result.accepted and result.status == 1:
            posted = True
        elif result.accepted and result.status == 7:
            posted = False
        fault = (
            config.fault or epoch.fault or decoded.fault or result.fault or (routing and hop.fault)
        )
        sent = not target or (
            reached and hop.lands and result.accepted and (result.status == 2 or result.status == 3)
        )
        ac.log(
            "info",
            "allgather_control_result",
            transfer.cycle,
            transfer.key_id,
            node,
            transfer.source,
            transfer.row,
            transfer.seq,
            transfer.epoch,
            reached,
            hop.egress if routing else 0,
            hop.lands if routing else False,
            hop.fault if routing else False,
            result.accepted,
            result.status,
            result.fault,
            result.active,
            result.counted,
            result.complete,
            result.pending,
            segment.address,
            segment.nbytes,
            stage,
            epoch.value,
            config.installed,
            config.admitted,
            config.fault,
            epoch.fault,
            decoded.expected_bytes,
            decoded.fault,
            configured,
            posted,
            sent,
            fault,
        )
        return MemberFeedback(
            epoch=epoch.value,
            configured=configured and admitted,
            posted=posted and result.active,
            sent=sent and admitted,
            complete=result.complete,
            drained=result.pending == 0,
            retired=not posted and not result.active,
            fault=fault,
        )

    return observe


def feedback_rows(
    m0: MemberFeedback,
    m1: MemberFeedback,
    m2: MemberFeedback,
    m3: MemberFeedback,
    m4: MemberFeedback,
    m5: MemberFeedback,
    m6: MemberFeedback,
    m7: MemberFeedback,
    m8: MemberFeedback,
    m9: MemberFeedback,
    m10: MemberFeedback,
    m11: MemberFeedback,
    m12: MemberFeedback,
    m13: MemberFeedback,
    m14: MemberFeedback,
    m15: MemberFeedback,
    m16: MemberFeedback,
    m17: MemberFeedback,
    m18: MemberFeedback,
    m19: MemberFeedback,
    m20: MemberFeedback,
    m21: MemberFeedback,
    m22: MemberFeedback,
    m23: MemberFeedback,
    m24: MemberFeedback,
    m25: MemberFeedback,
    m26: MemberFeedback,
    m27: MemberFeedback,
    m28: MemberFeedback,
    m29: MemberFeedback,
    m30: MemberFeedback,
    m31: MemberFeedback,
) -> ac.table[32, MemberFeedback]:
    rows: ac.table[32, MemberFeedback] = (
        m0,
        m1,
        m2,
        m3,
        m4,
        m5,
        m6,
        m7,
        m8,
        m9,
        m10,
        m11,
        m12,
        m13,
        m14,
        m15,
        m16,
        m17,
        m18,
        m19,
        m20,
        m21,
        m22,
        m23,
        m24,
        m25,
        m26,
        m27,
        m28,
        m29,
        m30,
        m31,
    )
    return rows


# ndf: implements=WSEMIG-CLOSEDLOOP-001
@ac.module
def ControlledAllGather(carrier: CarrierInput):
    stage: ac.u3 = 0
    key: ac.u1 = 0
    cursor: ac.u8 = 0
    epoch: ac.u8 = 0
    transfer = controlled_transfer(carrier.cycle, stage, key, cursor, epoch)
    command = Command(
        stage=stage,
        transfer=transfer,
        post_permit=carrier.post_permit,
        landing_permit=carrier.landing_permit,
    )
    hop0 = ForwardHop(line_input(transfer, 0))
    hop1 = ForwardHop(line_input(transfer, 1))
    hop2 = ForwardHop(line_input(transfer, 2))
    hop3 = ForwardHop(line_input(transfer, 3))
    hop4 = ForwardHop(line_input(transfer, 4))
    hop5 = ForwardHop(line_input(transfer, 5))
    hop6 = ForwardHop(line_input(transfer, 6))
    hop7 = ForwardHop(line_input(transfer, 7))
    hops = hop_rows(hop0, hop1, hop2, hop3, hop4, hop5, hop6, hop7)
    member0 = ControlledMember(command, hops, NODE=0)
    member1 = ControlledMember(command, hops, NODE=1)
    member2 = ControlledMember(command, hops, NODE=2)
    member3 = ControlledMember(command, hops, NODE=3)
    member4 = ControlledMember(command, hops, NODE=4)
    member5 = ControlledMember(command, hops, NODE=5)
    member6 = ControlledMember(command, hops, NODE=6)
    member7 = ControlledMember(command, hops, NODE=7)
    member8 = ControlledMember(command, hops, NODE=8)
    member9 = ControlledMember(command, hops, NODE=9)
    member10 = ControlledMember(command, hops, NODE=10)
    member11 = ControlledMember(command, hops, NODE=11)
    member12 = ControlledMember(command, hops, NODE=12)
    member13 = ControlledMember(command, hops, NODE=13)
    member14 = ControlledMember(command, hops, NODE=14)
    member15 = ControlledMember(command, hops, NODE=15)
    member16 = ControlledMember(command, hops, NODE=16)
    member17 = ControlledMember(command, hops, NODE=17)
    member18 = ControlledMember(command, hops, NODE=18)
    member19 = ControlledMember(command, hops, NODE=19)
    member20 = ControlledMember(command, hops, NODE=20)
    member21 = ControlledMember(command, hops, NODE=21)
    member22 = ControlledMember(command, hops, NODE=22)
    member23 = ControlledMember(command, hops, NODE=23)
    member24 = ControlledMember(command, hops, NODE=24)
    member25 = ControlledMember(command, hops, NODE=25)
    member26 = ControlledMember(command, hops, NODE=26)
    member27 = ControlledMember(command, hops, NODE=27)
    member28 = ControlledMember(command, hops, NODE=28)
    member29 = ControlledMember(command, hops, NODE=29)
    member30 = ControlledMember(command, hops, NODE=30)
    member31 = ControlledMember(command, hops, NODE=31)
    members = feedback_rows(
        member0,
        member1,
        member2,
        member3,
        member4,
        member5,
        member6,
        member7,
        member8,
        member9,
        member10,
        member11,
        member12,
        member13,
        member14,
        member15,
        member16,
        member17,
        member18,
        member19,
        member20,
        member21,
        member22,
        member23,
        member24,
        member25,
        member26,
        member27,
        member28,
        member29,
        member30,
        member31,
    )

    @ac.rule
    def advance() -> None:
        configured = True
        posted = True
        sent = True
        complete = True
        drained = True
        retired = True
        fault = False
        uniform = True
        for index in range(32):
            member = members[index]
            configured = configured and member.configured
            posted = posted and member.posted
            sent = sent and member.sent
            complete = complete and member.complete
            drained = drained and member.drained
            retired = retired and member.retired
            fault = fault or member.fault
            uniform = uniform and member.epoch == members[0].epoch
        ac.log(
            "info",
            "allgather_control_state",
            carrier.cycle,
            stage,
            key,
            cursor,
            epoch,
            configured,
            posted,
            sent,
            complete,
            drained,
            retired,
            fault,
            uniform,
        )
        if fault or not uniform:
            stage = 7
        elif stage == 0 and configured and drained and retired:
            stage = 1
        elif stage == 1:
            epoch = members[0].epoch
            stage = 2
        elif stage == 2 and posted:
            stage = 3
        elif stage == 3 and sent:
            if cursor == 255:
                stage = 4
            else:
                cursor = cursor + 1
        elif stage == 4 and complete and drained:
            stage = 5
        elif stage == 5 and retired and drained:
            if key == 0:
                key = 1
                cursor = 0
                stage = 1
            else:
                stage = 6
