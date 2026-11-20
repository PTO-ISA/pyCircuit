# ruff: noqa: N802, N803, F823, F841 -- pyCircuit declarations and captured state.
"""Finite Calendar ingress; capacities are mandatory consumer parameters.

One resident receive context is an explicit resource limit, not a Calendar
architecture default. Unmatched epochs share EARLY_SLOTS pending entries.
Matching early entries drain one per cycle before another request is accepted.
Faults are terminal until reset. No payload memory is represented here.
The 16-bit sequence field is a test carrier (0..65535), not a WSE header
specification. A producer must not wrap/reuse an identity in a resident context.
Dedup exhaustion terminates simulation with a model-capacity diagnostic; it
does not report a WSE protocol fault or wait for space that cannot be freed.
"""

import pycircuit as ac
from wse_calendar.receive_checks import ReceiveChecks


@ac.struct
class Segment:
    opcode: ac.u3
    epoch: ac.u8
    address: ac.u64
    nbytes: ac.u32
    source: ac.u6
    seq: ac.u16
    committed: bool
    self_sourced: bool


@ac.struct
class Request:
    # 0 idle, 1 post, 2 deliver, 3 retire.
    kind: ac.u2
    segment: Segment
    capacity: ac.u32
    expected: ac.u32
    compiled_expected: ac.u32


@ac.struct
class Account:
    opcode: ac.u3
    epoch: ac.u8
    dst: ac.u64
    capacity: ac.u32
    expected: ac.u32
    counted: ac.u32


@ac.struct
class Identity:
    valid: bool
    source: ac.u6
    seq: ac.u16


@ac.struct
class Pending:
    valid: bool
    segment: Segment


@ac.struct
class Result:
    accepted: bool
    # 0 idle, 1 posted, 2 counted, 3 duplicate, 4 self, 5 uncommitted,
    # 6 deferred, 7 retired, 8 resource backpressure, 9 fault.
    status: ac.u4
    fault: bool
    active: bool
    counted: ac.u32
    complete: bool
    pending: ac.u8


# ndf: implements=WSEMIG-RX-001
@ac.module
def ReceiveEngine(request: Request, *, EARLY_SLOTS: int, SEEN_SLOTS: int) -> Result:
    checks = ReceiveChecks()
    active: bool = False
    fault: bool = False
    account: Account = Account()
    pending = ac.table[EARLY_SLOTS, Pending](init=0)
    seen = ac.table[SEEN_SLOTS, Identity](init=0)

    @ac.method
    def account_segment(segment: Segment) -> ac.u4:
        status: ac.u4 = 0
        duplicate = False
        free = False
        slot: ac.u8 = 0
        for index in range(SEEN_SLOTS):
            entry = seen[index]
            if entry.valid and entry.source == segment.source and entry.seq == segment.seq:
                duplicate = True
            if not entry.valid and not free:
                free = True
                slot = index
        # Widen before addition: the comparison must not wrap at 64/32 bits.
        address_bytes: ac.bits[65] = segment.nbytes
        address_capacity: ac.bits[65] = account.capacity
        end: ac.bits[65] = segment.address
        end = end + address_bytes
        limit: ac.bits[65] = account.dst
        limit = limit + address_capacity
        count_bytes: ac.u33 = segment.nbytes
        count_capacity: ac.u33 = account.capacity
        total: ac.u33 = account.counted
        total = total + count_bytes
        if segment.address < account.dst or end > limit:
            fault = True
            status = 9
        elif not segment.committed:
            status = 5
        elif segment.self_sourced:
            status = 4
        elif duplicate:
            status = 3
        elif total > count_capacity:
            fault = True
            status = 9
        else:
            # Assertion failure aborts the whole transaction before publication.
            # Retain existing identities, count and the pending entry on failure.
            checks.require_identity_capacity(free)
            # slot starts at zero and only receives an in-range scan index.
            # Remainder preserves that value and supplies the access range proof.
            seen[slot % SEEN_SLOTS] = Identity(valid=True, source=segment.source, seq=segment.seq)
            account.counted = total[0:32]
            status = 2
        return status

    @ac.rule
    def step() -> Result:
        early_slots: ac.u16 = EARLY_SLOTS
        seen_slots: ac.u16 = SEEN_SLOTS
        assert early_slots > 0 and early_slots <= 255, "explicit early capacity 1..255"
        assert seen_slots > 0 and seen_slots <= 255, "explicit seen capacity 1..255"
        accepted = False
        status: ac.u4 = 0
        found = False
        drain_slot: ac.u8 = 0
        free = False
        free_slot: ac.u8 = 0
        for index in range(EARLY_SLOTS):
            entry = pending[index]
            if (
                active
                and entry.valid
                and entry.segment.opcode == account.opcode
                and entry.segment.epoch == account.epoch
                and not found
            ):
                found = True
                drain_slot = index
            if not entry.valid and not free:
                free = True
                free_slot = index
        if fault:
            status = 9
        elif found:
            # A capacity assertion aborts before pending removal can commit.
            status = account_segment(pending[drain_slot % EARLY_SLOTS].segment)
            pending[drain_slot % EARLY_SLOTS].valid = False
        elif request.kind == 1:
            if (
                request.segment.opcode == 0
                or request.segment.opcode == 7
                or request.segment.epoch == 0
                or request.expected > request.capacity
                or request.expected != request.compiled_expected
            ):
                accepted = True
                fault = True
                status = 9
            elif active:
                if (
                    account.opcode == request.segment.opcode
                    and account.epoch == request.segment.epoch
                    and account.counted < account.expected
                ):
                    accepted = True
                    fault = True
                    status = 9
                else:
                    status = 8
            else:
                accepted = True
                active = True
                account = Account(
                    opcode=request.segment.opcode,
                    epoch=request.segment.epoch,
                    dst=request.segment.address,
                    capacity=request.capacity,
                    expected=request.expected,
                    counted=0,
                )
                status = 1
        elif request.kind == 2:
            if request.segment.opcode == 0 or request.segment.opcode == 7:
                accepted = True
                fault = True
                status = 9
            elif (
                active
                and request.segment.opcode == account.opcode
                and request.segment.epoch == account.epoch
            ):
                status = account_segment(request.segment)
                accepted = True
            elif free:
                accepted = True
                pending[free_slot % EARLY_SLOTS] = Pending(valid=True, segment=request.segment)
                status = 6
            else:
                status = 8
        elif request.kind == 3:
            accepted = True
            if not active or account.counted < account.expected:
                fault = True
                status = 9
            else:
                active = False
                for index in range(SEEN_SLOTS):
                    seen[index].valid = False
                status = 7
        occupancy: ac.u8 = 0
        for index in range(EARLY_SLOTS):
            if pending[index].valid:
                occupancy = occupancy + 1
        return Result(
            accepted=accepted,
            status=status,
            fault=fault,
            active=active,
            counted=account.counted,
            complete=active and account.counted >= account.expected,
            pending=occupancy,
        )

    return step
