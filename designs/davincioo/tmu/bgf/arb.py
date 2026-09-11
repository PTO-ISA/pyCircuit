# ndf: node=ndf://davincioo/DAV-TMU-BGF-ARB-0001
"""Parent-composed grant arbitration seam for one PE's block-group fabric.

ARB owns the conflict decision for one PE's private banks; RQ/WQ residency and
bank state stay outside this leaf.  This file describes the typed arbitration
seam and the baseline priority order, not the aging or cursor storage.

One elaboration of ``bgf_arb_system`` covers exactly one PE.  The four PE
control flows in designs/davincioo/ARCHITECTURE.md are physically independent
and own private bank groups, so PE identity is expressed by instantiating this
system once per PE rather than by a routing dimension inside it.  No age,
cursor, deficit counter, or retained grant is shared across PEs.

Within one PE the contender set of one bank is the six ``(source_class, path)``
class streams that designs/davincioo/tmu/bgf/rq.py and
designs/davincioo/tmu/bgf/wq.py hold.  ``BankGrant`` is one common
request/grant record so read and write streams enter the same conflict
scheduler without dropping identity, mask, or payload fields.

ARB owns the entire bank dimension.  The six class streams arrive with no bank
structure at all; this leaf fans each of them across ``BANK_PARTITIONS`` and
then resolves each bank's contenders.  RQ and WQ are deliberately bank-blind,
so the crossbar exists once, here, next to the conflict decision it serves.

``bank`` is a payload field because of that split: designs/davincioo/tmu/bgf/
map.py decodes it from ``cell_key`` and it must survive RQ/WQ residency to be
read here.  ``granted_bank`` remains separate and is written as a compile-time
constant, so a grant records which tree accepted it rather than which tree it
requested.  ``row`` is the placement coordinate that MAP computes and BANK
consumes; ``GrantCancel`` carries neither field because ``cell_key`` locates
the target and the selector is derivable from it.
"""

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_bgf import (
    BANK_INDEX_MASK,
    BANK_PARTITIONS,
)


@ac.struct
class BankGrant:
    sequence: ac.u16
    source_class: ac.u8
    requester: ac.u8
    flow_id: ac.u16
    launch_generation: ac.u16
    thread_id: ac.u16
    block_id: ac.u16
    operation_id: ac.u16
    tile_id: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    request_id: ac.u16
    cell_key: ac.u16
    bank: ac.u16
    row: ac.u16
    response_route: ac.u8
    payload: ac.u64
    ordering_tag: ac.u16
    definedness_tag: ac.u16
    granted_bank: ac.u8
    is_write: bool
    all_or_none: bool
    escalated: bool
    accepted: bool
    cancelled: bool


@ac.struct
class GrantCancel:
    sequence: ac.u16
    source_class: ac.u8
    requester: ac.u8
    flow_id: ac.u16
    launch_generation: ac.u16
    thread_id: ac.u16
    block_id: ac.u16
    operation_id: ac.u16
    tile_id: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    request_id: ac.u16
    cell_key: ac.u16
    accepted: bool
    cancelled: bool
    acknowledged: bool


@ac.system
def bgf_arb_system(
    cube_write: BankGrant,
    cube_read: BankGrant,
    vec_write: BankGrant,
    vec_read: BankGrant,
    tlsu_write: BankGrant,
    tlsu_read: BankGrant,
    grant_cancel: GrantCancel,
) -> tuple[
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    BankGrant,
    GrantCancel,
]:
    """Grant at most one access per bank per cycle for one PE.

    Each of the six bank-blind class streams is first fanned across
    ``BANK_PARTITIONS`` by the ``bank`` field MAP decoded, giving one buffered
    edge per ``(source_class, path, bank)``.  Those edges are what make banks
    independent: a bank blocked downstream holds only its own edges, and the
    other banks keep draining.  Independence stops at the fan-out input, where
    a stream whose head targets a full bank blocks that stream's later
    requests; that head-of-line limit is inherent to fanning one stream out and
    is unchanged by where the fan-out is drawn.

    ``BANK_PARTITIONS`` independent bank trees then merge the six edges of
    their bank with a ``priority`` merge, so operand order is the grant order:
    ``cube_w``, ``cube_r``, ``vec_w``, ``vec_r``, ``tlsu_w``, ``tlsu_r``.
    Writes outrank reads of the same class and class rank dominates the
    read/write distinction, exactly as arb.md states.

    The route key re-masks ``bank`` rather than trusting its range, which keeps
    ``route_selector_out_of_range`` structurally unreachable instead of
    dependent on an upstream promise.  Masking is not a second placement
    decision: the value still comes from MAP, and row bounds are a separate MAP
    result (``out_of_range``) that this leaf does not act on.

    The selected transaction is annotated with the accepted bank and forked
    into the client and XBAR publication paths, so both consumers observe the
    same accepted grant.  ``grant_to_xbar`` in arb.md is elaborated per bank
    rather than as one merged port, so that a bank blocked by XBAR cannot stall
    the other banks of this PE.

    Two policy tiers of arb.md are deliberately absent from this seam because
    they need state this leaf does not own: the aging escalation to ``granted``
    and the committed round-robin cursor over the escalated contender set.
    ``escalated`` and ``accepted`` travel in ``BankGrant`` so the later
    stateful owner can add that policy without changing the transaction ABI.

    ``grant_cancel`` is acknowledged on a separate path that preserves the
    generation-qualified cancellation payload.  Whether a retained grant is
    still cancellable depends on the retained-grant register, which is likewise
    pending, so this seam only marks the acknowledgement.
    """

    (
        cube_write_b0,
        cube_write_b1,
        cube_write_b2,
        cube_write_b3,
        cube_write_b4,
        cube_write_b5,
        cube_write_b6,
        cube_write_b7,
    ) = cube_write.route(
        outputs=BANK_PARTITIONS,
        key=lambda grant: grant.bank & BANK_INDEX_MASK,
        depth=1,
        latency=1,
    )
    (
        cube_read_b0,
        cube_read_b1,
        cube_read_b2,
        cube_read_b3,
        cube_read_b4,
        cube_read_b5,
        cube_read_b6,
        cube_read_b7,
    ) = cube_read.route(
        outputs=BANK_PARTITIONS,
        key=lambda grant: grant.bank & BANK_INDEX_MASK,
        depth=1,
        latency=1,
    )
    (
        vec_write_b0,
        vec_write_b1,
        vec_write_b2,
        vec_write_b3,
        vec_write_b4,
        vec_write_b5,
        vec_write_b6,
        vec_write_b7,
    ) = vec_write.route(
        outputs=BANK_PARTITIONS,
        key=lambda grant: grant.bank & BANK_INDEX_MASK,
        depth=1,
        latency=1,
    )
    (
        vec_read_b0,
        vec_read_b1,
        vec_read_b2,
        vec_read_b3,
        vec_read_b4,
        vec_read_b5,
        vec_read_b6,
        vec_read_b7,
    ) = vec_read.route(
        outputs=BANK_PARTITIONS,
        key=lambda grant: grant.bank & BANK_INDEX_MASK,
        depth=1,
        latency=1,
    )
    (
        tlsu_write_b0,
        tlsu_write_b1,
        tlsu_write_b2,
        tlsu_write_b3,
        tlsu_write_b4,
        tlsu_write_b5,
        tlsu_write_b6,
        tlsu_write_b7,
    ) = tlsu_write.route(
        outputs=BANK_PARTITIONS,
        key=lambda grant: grant.bank & BANK_INDEX_MASK,
        depth=1,
        latency=1,
    )
    (
        tlsu_read_b0,
        tlsu_read_b1,
        tlsu_read_b2,
        tlsu_read_b3,
        tlsu_read_b4,
        tlsu_read_b5,
        tlsu_read_b6,
        tlsu_read_b7,
    ) = tlsu_read.route(
        outputs=BANK_PARTITIONS,
        key=lambda grant: grant.bank & BANK_INDEX_MASK,
        depth=1,
        latency=1,
    )

    bank0_selected = ac.merge(
        cube_write_b0,
        cube_read_b0,
        vec_write_b0,
        vec_read_b0,
        tlsu_write_b0,
        tlsu_read_b0,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank0_granted = bank0_selected.apply(
        lambda grant: grant.with_fields(granted_bank=0, accepted=True),
        depth=1,
        latency=1,
    )
    bank0_client, bank0_xbar = bank0_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    bank1_selected = ac.merge(
        cube_write_b1,
        cube_read_b1,
        vec_write_b1,
        vec_read_b1,
        tlsu_write_b1,
        tlsu_read_b1,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank1_granted = bank1_selected.apply(
        lambda grant: grant.with_fields(granted_bank=1, accepted=True),
        depth=1,
        latency=1,
    )
    bank1_client, bank1_xbar = bank1_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    bank2_selected = ac.merge(
        cube_write_b2,
        cube_read_b2,
        vec_write_b2,
        vec_read_b2,
        tlsu_write_b2,
        tlsu_read_b2,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank2_granted = bank2_selected.apply(
        lambda grant: grant.with_fields(granted_bank=2, accepted=True),
        depth=1,
        latency=1,
    )
    bank2_client, bank2_xbar = bank2_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    bank3_selected = ac.merge(
        cube_write_b3,
        cube_read_b3,
        vec_write_b3,
        vec_read_b3,
        tlsu_write_b3,
        tlsu_read_b3,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank3_granted = bank3_selected.apply(
        lambda grant: grant.with_fields(granted_bank=3, accepted=True),
        depth=1,
        latency=1,
    )
    bank3_client, bank3_xbar = bank3_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    bank4_selected = ac.merge(
        cube_write_b4,
        cube_read_b4,
        vec_write_b4,
        vec_read_b4,
        tlsu_write_b4,
        tlsu_read_b4,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank4_granted = bank4_selected.apply(
        lambda grant: grant.with_fields(granted_bank=4, accepted=True),
        depth=1,
        latency=1,
    )
    bank4_client, bank4_xbar = bank4_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    bank5_selected = ac.merge(
        cube_write_b5,
        cube_read_b5,
        vec_write_b5,
        vec_read_b5,
        tlsu_write_b5,
        tlsu_read_b5,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank5_granted = bank5_selected.apply(
        lambda grant: grant.with_fields(granted_bank=5, accepted=True),
        depth=1,
        latency=1,
    )
    bank5_client, bank5_xbar = bank5_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    bank6_selected = ac.merge(
        cube_write_b6,
        cube_read_b6,
        vec_write_b6,
        vec_read_b6,
        tlsu_write_b6,
        tlsu_read_b6,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank6_granted = bank6_selected.apply(
        lambda grant: grant.with_fields(granted_bank=6, accepted=True),
        depth=1,
        latency=1,
    )
    bank6_client, bank6_xbar = bank6_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    bank7_selected = ac.merge(
        cube_write_b7,
        cube_read_b7,
        vec_write_b7,
        vec_read_b7,
        tlsu_write_b7,
        tlsu_read_b7,
        policy="priority",
        depth=2,
        latency=1,
    )
    bank7_granted = bank7_selected.apply(
        lambda grant: grant.with_fields(granted_bank=7, accepted=True),
        depth=1,
        latency=1,
    )
    bank7_client, bank7_xbar = bank7_granted.fork(
        outputs=2,
        depth=1,
        latency=1,
    )

    grant_cancel_ack = grant_cancel.apply(
        lambda cancel: cancel.with_fields(acknowledged=True),
        depth=1,
        latency=1,
    )
    return (
        bank0_client,
        bank1_client,
        bank2_client,
        bank3_client,
        bank4_client,
        bank5_client,
        bank6_client,
        bank7_client,
        bank0_xbar,
        bank1_xbar,
        bank2_xbar,
        bank3_xbar,
        bank4_xbar,
        bank5_xbar,
        bank6_xbar,
        bank7_xbar,
        grant_cancel_ack,
    )
