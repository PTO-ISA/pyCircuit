# ruff: noqa: N802, F841 -- pyCircuit declarations and static child instances.
"""Serialized functional FFN AllGather over all 32 members of the 40-node carrier.

Eight stateless hop slots serve the one active row/column group each cycle.
Every member keeps its own receive state across both phases. This is a test
carrier, not a 32-way concurrent hardware network or a link-latency model.
"""

import pycircuit as ac
from wse_calendar.forward import HopInput, HopResult
from wse_calendar.receive import ReceiveEngine, Request, Segment


@ac.struct
class Transfer:
    cycle: ac.u10
    key_id: ac.u1
    source: ac.u6
    row: ac.u3
    seq: ac.u16
    epoch: ac.u8
    route: ac.bits[80]
    address: ac.u64
    nbytes: ac.u32
    capacity: ac.u32
    expected: ac.u32
    post: bool
    retire: bool
    send: bool
    base: ac.u6
    stride: ac.u6
    source_rank: ac.u3


def route_rows() -> ac.table[64, ac.bits[80]]:
    # ffn_example(): key 0 sources 0..31, then key 1 sources 0..31.
    # Rows 0 and 32 retain the two published Calendar hexadecimal vectors.
    rows: ac.table[64, ac.bits[80]] = (
        0x0000000000000000FFFE,
        0x0000000000000000FFFB,
        0x0000000000000000FFEF,
        0x0000000000000000FFBF,
        0x0000000000000000FEFF,
        0x0000000000000000FBFF,
        0x0000000000000000EFFF,
        0x0000000000000000BFFF,
        0x000000000000FFFE0000,
        0x000000000000FFFB0000,
        0x000000000000FFEF0000,
        0x000000000000FFBF0000,
        0x000000000000FEFF0000,
        0x000000000000FBFF0000,
        0x000000000000EFFF0000,
        0x000000000000BFFF0000,
        0x00000000FFFE00000000,
        0x00000000FFFB00000000,
        0x00000000FFEF00000000,
        0x00000000FFBF00000000,
        0x00000000FEFF00000000,
        0x00000000FBFF00000000,
        0x00000000EFFF00000000,
        0x00000000BFFF00000000,
        0x0000FFFE000000000000,
        0x0000FFFB000000000000,
        0x0000FFEF000000000000,
        0x0000FFBF000000000000,
        0x0000FEFF000000000000,
        0x0000FBFF000000000000,
        0x0000EFFF000000000000,
        0x0000BFFF000000000000,
        0x00000003000300030002,
        0x0000000C000C000C0008,
        0x00000030003000300020,
        0x000000C000C000C00080,
        0x00000300030003000200,
        0x00000C000C000C000800,
        0x00003000300030002000,
        0x0000C000C000C0008000,
        0x00000003000300020003,
        0x0000000C000C0008000C,
        0x00000030003000200030,
        0x000000C000C0008000C0,
        0x00000300030002000300,
        0x00000C000C0008000C00,
        0x00003000300020003000,
        0x0000C000C0008000C000,
        0x00000003000200030003,
        0x0000000C0008000C000C,
        0x00000030002000300030,
        0x000000C0008000C000C0,
        0x00000300020003000300,
        0x00000C0008000C000C00,
        0x00003000200030003000,
        0x0000C0008000C000C000,
        0x00000002000300030003,
        0x00000008000C000C000C,
        0x00000020003000300030,
        0x0000008000C000C000C0,
        0x00000200030003000300,
        0x000008000C000C000C00,
        0x00002000300030003000,
        0x00008000C000C000C000,
    )
    return rows


def transfer_for(cycle: ac.u10) -> Transfer:
    column = cycle >= 258
    post = cycle == 0 or cycle == 258
    retire = cycle == 257 or cycle == 515
    send = not post and not retire
    offset: ac.u10 = 0 if not send else cycle - 259 if column else cycle - 1
    source: ac.u6 = offset[3:8]
    row = offset[0:3]
    source_rank = source[3:6] if column else source[0:3]
    route_index: ac.u6 = source
    if column:
        route_index = route_index + 32
    routes = route_rows()
    seq: ac.u16 = row
    if column:
        seq = seq + 8
    nbytes: ac.u32 = 1536 if column else 192
    stride_bytes: ac.u64 = 6144 if column else 1536
    rank_wide: ac.u64 = source_rank
    row_wide: ac.u64 = row
    bytes_wide: ac.u64 = nbytes
    address = 4096 + row_wide * stride_bytes + rank_wide * bytes_wide
    return Transfer(
        cycle=cycle,
        key_id=1 if column else 0,
        source=source,
        row=row,
        seq=seq,
        epoch=2 if column else 1,
        route=routes[route_index],
        address=address,
        nbytes=nbytes,
        capacity=49152 if column else 12288,
        expected=36864 if column else 10752,
        post=post,
        retire=retire,
        send=send,
        base=source & 7 if column else source & 56,
        stride=8 if column else 1,
        source_rank=source_rank,
    )


def line_input(transfer: Transfer, position: ac.u3) -> HopInput:
    # Only positions 0..3 participate in a column group. Unused physical hop
    # slots reuse those inputs; member selection never addresses them in phase C.
    slot: ac.u6 = position
    if transfer.key_id == 1:
        slot = slot % 4
    node = transfer.base + transfer.stride * slot
    rank: ac.u6 = transfer.source_rank
    ingress = node + transfer.stride if slot < rank else node - transfer.stride
    if node == transfer.source:
        ingress = 0
    return HopInput(
        route=transfer.route,
        node=node,
        has_ingress=node != transfer.source,
        ingress=ingress,
    )


def hop_rows(
    h0: HopResult,
    h1: HopResult,
    h2: HopResult,
    h3: HopResult,
    h4: HopResult,
    h5: HopResult,
    h6: HopResult,
    h7: HopResult,
) -> ac.table[8, HopResult]:
    rows: ac.table[8, HopResult] = (h0, h1, h2, h3, h4, h5, h6, h7)
    return rows


def path_reached(hops: ac.table[8, HopResult], transfer: Transfer, target: ac.u3) -> bool:
    # The fixture's ROW/COL group is a line. Every required edge must be present
    # in its upstream ForwardHop output; group membership alone is insufficient.
    reached = True
    for index in range(8):
        rank: ac.u3 = index
        rank_wide: ac.u6 = index
        if rank >= transfer.source_rank and rank < target:
            peer = transfer.base + transfer.stride * (rank_wide + 1)
            reached = reached and not hops[index].fault and (hops[index].egress >> peer) & 1 != 0
        elif rank <= transfer.source_rank and rank > target:
            peer = transfer.base + transfer.stride * (rank_wide - 1)
            reached = reached and not hops[index].fault and (hops[index].egress >> peer) & 1 != 0
    return reached


# ndf: implements=WSEMIG-ALLGATHER-001
@ac.module
def AllGatherMember(transfer: Transfer, hops: ac.table[8, HopResult], node: ac.u6):
    rank = node[3:6] if transfer.key_id == 1 else node[0:3]
    same_group = (
        node[0:3] == transfer.source[0:3]
        if transfer.key_id == 1
        else node[3:6] == transfer.source[3:6]
    )
    routing = transfer.send and same_group
    hop = hops[rank]
    reached = routing and path_reached(hops, transfer, rank)
    segment = Segment(
        opcode=1,
        epoch=transfer.epoch,
        address=transfer.address,
        nbytes=transfer.nbytes,
        source=transfer.source,
        seq=transfer.seq,
        committed=True,
        self_sourced=node == transfer.source,
    )
    request = Request(
        kind=1 if transfer.post else 3 if transfer.retire else 2 if reached and hop.lands else 0,
        segment=segment,
        capacity=transfer.capacity,
        expected=transfer.expected,
        compiled_expected=transfer.expected,
    )
    # Core Noc.send creates one identity per row: 7 remote sources * 8 rows in B;
    # C uses 3 * 8 identities. Retirement clears the same resident table.
    result = ReceiveEngine(request, EARLY_SLOTS=2, SEEN_SLOTS=56)

    @ac.rule
    def observe() -> None:
        ac.log(
            "info",
            "allgather_result",
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
        )
