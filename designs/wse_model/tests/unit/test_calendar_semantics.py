"""Collective codes, the inseparable key pair, and epoch accounting."""

from __future__ import annotations

import pytest

from wse_model.calendar.collective import (
    ALIGNMENT_SEMAPHORE_WIDTH_BITS,
    OPCODE_RESERVED,
    OPCODE_WIDTH_BITS,
    RED_OP_WIDTH_BITS,
    AlignmentDomain,
    Collective,
    RedOp,
    ReductionElementType,
)
from wse_model.calendar.epoch import (
    EPOCH_TAG_MAX_BITS,
    EPOCH_TAG_MIN_BITS,
    EpochCounter,
    EpochTracker,
    RecvContext,
)
from wse_model.calendar.key import CalendarKeyRef, CalendarKeyRegistry, GroupRole, RouteKey
from wse_model.calendar.route_bits import RouteBits
from wse_model.errors import CalendarError, WseModelError

pytestmark = pytest.mark.unit


def _key(phase: str = "phase_b", collective: Collective = Collective.ALL_GATHER) -> RouteKey:
    return RouteKey(
        program_id="prog",
        kernel_id="kern",
        phase_id=phase,
        collective=collective,
        group_role=GroupRole(kind="ROW", axis="my_row"),
    )


# -- opcodes ---------------------------------------------------------------


def test_opcode_is_three_bits_and_zero_seven_are_reserved() -> None:
    """Calendar §2.3: 3 bit so six collectives fit with illegal code points."""
    assert OPCODE_WIDTH_BITS == 3
    assert OPCODE_RESERVED == {0, 7}
    assert Collective.ALL_GATHER.opcode == 1
    assert Collective.REDUCE.opcode == 2
    assert Collective.GATHER.opcode == 6


def test_only_allgather_and_reduce_are_implemented_in_the_first_phase() -> None:
    implemented = {c for c in Collective if c.is_implemented}
    assert implemented == {Collective.ALL_GATHER, Collective.REDUCE}


def test_reduction_collectives_are_identified() -> None:
    assert Collective.ALL_REDUCE.is_reduction
    assert Collective.REDUCE_SCATTER.is_reduction
    assert not Collective.ALL_GATHER.is_reduction
    assert not Collective.SCATTER.is_reduction


def test_from_opcode_rejects_values_outside_three_bits() -> None:
    with pytest.raises(WseModelError):
        Collective.from_opcode(8)


def test_red_op_width_and_members() -> None:
    assert RED_OP_WIDTH_BITS == 3
    assert [op.name for op in RedOp] == ["NONE", "SUM", "MAX", "MIN", "PROD"]
    assert not RedOp.NONE.is_active


def test_reduction_element_type_carries_the_code_but_refuses_to_name_it() -> None:
    """Open item C-3: the seven vtype_t values are not fixed by the sources."""
    element = ReductionElementType(3)
    assert element == 3
    assert element.code == 3
    with pytest.raises(WseModelError):
        ReductionElementType(8)
    from wse_model.errors import OpenItemError

    with pytest.raises(OpenItemError) as excinfo:
        _ = element.name
    assert "C-3" in str(excinfo.value)


# -- alignment -------------------------------------------------------------


def test_alignment_domain_capacity_is_four_bits_by_default() -> None:
    domain = AlignmentDomain(1)
    assert ALIGNMENT_SEMAPHORE_WIDTH_BITS == 4
    assert domain.capacity == 15
    assert domain.check_width(15)
    assert not domain.check_width(32)


def test_alignment_domain_faults_on_wraparound() -> None:
    """HW-7: a 4 bit semaphore cannot hold a 32 core FFN phase B report."""
    domain = AlignmentDomain(1)
    with pytest.raises(WseModelError):
        domain.report(32)


def test_alignment_domain_width_requirement_matches_hw7() -> None:
    # ceil(log2(32 + 1)) == 6, hence HW-7's "at least 6 bit".
    assert AlignmentDomain.required_width_for(32) == 6
    assert AlignmentDomain.required_width_for(40) == 6
    assert AlignmentDomain.required_width_for(15) == 4
    assert AlignmentDomain.required_width_for(16) == 5

    wide = AlignmentDomain(1, width_bits=6)
    assert wide.check_width(32)
    assert not wide.check_width(64)
    wide.report(32)
    assert wide.arrivals == 32
    assert wide.required_width_bits == 6


def test_alignment_domain_rejects_reserved_opcodes() -> None:
    for opcode in (0, 7):
        with pytest.raises(WseModelError):
            AlignmentDomain(opcode)


# -- key ref ---------------------------------------------------------------


def test_key_ref_carries_the_identity_derived_opcode() -> None:
    key = _key()
    assert key.opcode == Collective.ALL_GATHER.opcode
    ref = CalendarKeyRef(key_id=0, opcode=Collective.ALL_GATHER, route_version=1)
    assert ref.opcode_value == 1
    ref.require_version(route_version=1)


def test_key_ref_rejects_a_reserved_opcode() -> None:
    with pytest.raises(CalendarError):
        CalendarKeyRef(key_id=0, opcode=Collective.INVALID)


def test_key_ref_requires_red_op_exactly_for_reductions() -> None:
    with pytest.raises(CalendarError):
        CalendarKeyRef(key_id=0, opcode=Collective.ALL_REDUCE, red_op=RedOp.NONE)
    with pytest.raises(CalendarError):
        CalendarKeyRef(key_id=0, opcode=Collective.ALL_GATHER, red_op=RedOp.SUM)
    ok = CalendarKeyRef(key_id=0, opcode=Collective.ALL_REDUCE, red_op=RedOp.SUM)
    assert ok.red_op is RedOp.SUM


def test_key_ref_version_check_faults_on_mismatch() -> None:
    from wse_model.errors import VersionMismatchError

    ref = CalendarKeyRef(key_id=0, opcode=Collective.ALL_GATHER, route_version=1)
    with pytest.raises(VersionMismatchError):
        ref.require_version(route_version=2)


def test_registry_assigns_stable_key_ids() -> None:
    registry = CalendarKeyRegistry()
    first = registry.register(_key("phase_b"))
    second = registry.register(_key("phase_c"))
    assert (first, second) == (0, 1)
    # Re-registering the same identity is idempotent.
    assert registry.register(_key("phase_b")) == 0
    assert registry.key_count == 2


def test_registry_defends_invariant_o1() -> None:
    """One identity maps to exactly one (keyId, opcode) pair, and vice versa."""
    registry = CalendarKeyRegistry()
    registry.register(_key("phase_b"))
    with pytest.raises(CalendarError):
        registry.register(_key("phase_c"), key_id=0)
    with pytest.raises(CalendarError):
        registry.register(_key("phase_b"), key_id=1)


def test_registry_enforces_the_key_budget() -> None:
    registry = CalendarKeyRegistry()
    for index in range(CalendarKeyRegistry.MAX_KEYS):
        registry.register(_key(f"phase_{index}"))
    with pytest.raises(CalendarError):
        registry.register(_key("one_too_many"))


def test_registry_resolves_a_key_ref_with_the_identity_opcode() -> None:
    registry = CalendarKeyRegistry()
    registry.register(_key("phase_b"))
    ref = registry.key_ref(_key("phase_b"), route_version=3)
    assert ref.key_id == 0
    assert ref.opcode is Collective.ALL_GATHER
    assert ref.red_op is RedOp.NONE
    assert ref.route_version == 3


# -- epoch -----------------------------------------------------------------


def test_epoch_counter_starts_at_one() -> None:
    counter = EpochCounter(1)
    assert counter.current == 0
    assert counter.next() == 1
    assert counter.next() == 2
    assert counter.current == 2


def test_epoch_counter_wraparound_requires_a_drain() -> None:
    """C-5 contract 7: the previous epoch's traffic must be drained first."""
    counter = EpochCounter(1, width_bits=EPOCH_TAG_MIN_BITS)
    assert counter.capacity == 255
    for _ in range(255):
        value = counter.next()
    assert value == 255
    with pytest.raises(WseModelError):
        counter.next(drained=False)
    assert counter.next(drained=True) == 1


def test_epoch_tag_width_is_bounded_by_the_open_item_range() -> None:
    with pytest.raises(WseModelError):
        EpochCounter(1, width_bits=EPOCH_TAG_MAX_BITS + 1)
    assert EPOCH_TAG_MIN_BITS == 8
    assert EPOCH_TAG_MAX_BITS == 16


def test_epoch_counter_rejects_reserved_opcodes() -> None:
    for opcode in (0, 7):
        with pytest.raises(WseModelError):
            EpochCounter(opcode)


def test_tracker_detects_spmd_divergence() -> None:
    """Invariant E1: members must call a collective the same number of times."""
    local = EpochTracker()
    peer = EpochTracker()
    local.next_epoch(1)
    peer.next_epoch(1)
    assert local.assert_spmd_consistent(1, {1: peer}) == 1
    peer.next_epoch(1)
    with pytest.raises(WseModelError) as excinfo:
        local.assert_spmd_consistent(1, {1: peer})
    assert "E1" in str(excinfo.value)


def test_recv_context_key_is_opcode_and_epoch() -> None:
    context = RecvContext(opcode=1, epoch=3, aicore=0, kernel_id="kern")
    other = RecvContext(opcode=1, epoch=3, aicore=7, kernel_id="kern")
    assert context.matches(other)
    assert not context.matches(RecvContext(opcode=1, epoch=4, aicore=0, kernel_id="kern"))


def test_recv_context_rejects_epoch_zero() -> None:
    with pytest.raises(WseModelError):
        RecvContext(opcode=1, epoch=0, aicore=0, kernel_id="kern")


def test_route_bits_helper_is_unused_but_importable() -> None:
    """Guard against an accidental wide-import regression in this module."""
    assert RouteBits.from_int(0, 40).width_bytes == 10
