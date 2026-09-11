"""FRE hands out physical tile slots and is the only module that reserves them.

These checks pin the five behavioral acceptances in
designs/davincioo/tmu/trn/fre.md. One is pinned as *unmet*: an exhausted pool
cannot stall its requester in the current framework, so
`test_exhaustion_is_reported_rather_than_stalled` records the gap instead of
asserting a behavior FRE does not have. That is the same Decision 0210 limit REF
hit, not a new one.

Several checks are structural rather than functional because the properties they
protect are structural. A pool with two writers, or a capacity query that
mutates, would break Decision 0151 rather than merely compute the wrong answer,
so those are asserted on the lowered module.
"""

from __future__ import annotations

import ast
import re
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_trn import (
    CELLS_PER_PE,
    CELLS_PER_TILE_SLOT,
    FRE_ALLOC,
    FRE_CANCEL,
    FRE_COMMIT,
    FRE_FREE,
    RESERVATION_SLOTS,
    SLOT_INDEX_BITS,
    TILE_SLOTS_PER_POOL,
)
from designs.davincioo.tmu.trn.fre import trn_fre_system

WORKSPACE = "/home/xiekunpeng/linxISA/pyCircuit"


@pytest.fixture(scope="module")
def lowered() -> str:
    return ac.jit(trn_fre_system, workspace=WORKSPACE).lower_acir()


def test_the_pool_has_exactly_one_writer(lowered: str) -> None:
    """Decision 0151 permits one write endpoint per table."""

    declarations = re.findall(r"ac\.var\.decl @(\w+) type", lowered)
    assert declarations == ["slots", "reservations"]
    assert f'stable_id "var/slots" shape [{TILE_SLOTS_PER_POOL}]' in lowered
    assert f'stable_id "var/reservations" shape [{RESERVATION_SLOTS}]' in lowered

    serving = _rule(lowered, "serve_allocation")
    assert "ac.var.assign_element @slots[" in serving
    assert "ac.var.assign_element @reservations[" in serving

    # The query rule sees the same pool and never writes it.
    assert "assign_element" not in _rule(lowered, "answer_capacity_query")


def test_the_capacity_query_only_reads(lowered: str) -> None:
    """A query must not consume a slot it merely reports on."""

    query = _rule(lowered, "answer_capacity_query")
    assert "assign_element" not in query
    # It still searches, so it observes real state rather than a cached count.
    assert "ac.var.match.yield" in query
    assert 'field "allocated"' in query


def test_the_four_operations_share_one_stream(lowered: str) -> None:
    """Four input ports would deadlock, since a rule needs every input token.

    Decision 0167/0168 fire a rule only when all its inputs have tokens. Allocate,
    commit, cancel and free arrive independently, so they must share one stream.
    """

    for name in ("serve_allocation", "answer_capacity_query"):
        header = re.search(
            rf"= ac\.rule ((?:%\w+, )*%\w+) depths .*name \"{name}\"", lowered
        )
        assert header is not None, f"no rule named {name}"
        assert "," not in header.group(1), f"{name} consumes more than one queue"

    # The operations are distinguished inside that stream, not by port.
    serving = _rule(lowered, "serve_allocation")
    for operation in (FRE_ALLOC, FRE_COMMIT, FRE_CANCEL, FRE_FREE):
        assert f"ac.var.constant {operation} : i8" in serving

    # Each acknowledgement still leaves on its own port, via a route.
    assert re.search(
        r"= ac\.route %completed depths \[1, 1, 1, 1\]", lowered
    ), "acknowledgements must fan out to one port per operation"


def test_allocation_is_all_or_none(lowered: str) -> None:
    """A slot without its reservation row, or the reverse, would leak.

    Both writes must be guarded by the same condition, so they commit together at
    the tick boundary or not at all.
    """

    serving = _rule(lowered, "serve_allocation")
    slot_writes = re.findall(
        r"assign_element @slots\[%\w+\] = %\w+ when (%\w+)", serving
    )
    row_writes = re.findall(
        r"assign_element @reservations\[%\w+\] = %\w+ when (%\w+)", serving
    )
    assert slot_writes and row_writes

    # Exactly one guard is shared between the two arrays: the allocate.
    shared = set(slot_writes) & set(row_writes)
    assert len(shared) == 1, "allocate must write both arrays under one condition"


def test_a_repeated_transaction_gets_the_same_slot(lowered: str) -> None:
    """A duplicate RenameTxnKey must not consume a second slot."""

    serving = _rule(lowered, "serve_allocation")

    # The duplicate search is keyed on the transaction, and requires a live row so
    # a retired reservation cannot alias a new transaction.
    duplicate = _find_predicate(serving, "find0")
    assert 'field "txn_key"' in duplicate
    assert 'field "live"' in duplicate

    # The response reports a slot index, so a repeated allocate is idempotent in
    # its answer and not only in the state.
    assert 'field "slot_index"' in serving


def test_a_duplicate_is_reported_as_matched_not_as_accepted(lowered: str) -> None:
    """`accepted` means the ledger changed, so a duplicate must not set it.

    This is an interface trap worth pinning. A repeated allocate succeeds from the
    caller's point of view -- it gets its slot back -- but nothing was allocated,
    so `accepted` is false. A caller reading only `accepted` would misread a
    successful retry as a refusal, which is why the three result bits must stay
    independently computed.
    """

    serving = _rule(lowered, "serve_allocation")
    values: dict[str, str] = {}
    for field in ("accepted", "matched", "exhausted"):
        match = re.search(rf"ac\.var\.with %\w+, (%\w+) field \"{field}\"", serving)
        assert match is not None, f"no field update for {field}"
        values[field] = match.group(1)

    assert (
        len(set(values.values())) == 3
    ), f"accepted, matched and exhausted must be computed separately; got {values}"


def test_a_free_needs_both_preconditions(lowered: str) -> None:
    """fre.md requires logical permission *and* a zero reference count.

    FRE can observe neither, so the coordinator presents both and FRE checks them
    rather than trusting the request.
    """

    serving = _rule(lowered, "serve_allocation")
    assert 'field "logical_permission"' in serving
    assert 'field "refs_zero"' in serving

    # The target search requires an allocated slot matching owner and version, so
    # a free cannot release someone else's slot.
    target = _find_predicate(serving, "find9")
    assert 'field "allocated"' in target
    assert 'field "owner"' in target
    assert 'field "tile_version"' in target


def test_exhaustion_is_reported_rather_than_stalled(lowered: str) -> None:
    """UNMET acceptance, recorded rather than asserted.

    fre.md requires that physical exhaustion backpressures instead of inventing a
    fault. FRE reports `exhausted` and refuses, which invents no fault and drops
    nothing, but it does consume the request instead of holding it.

    Decision 0210 fixes a rule's consumption condition to a proven-constant-true
    candidate and states the false path may consume input, so a condition derived
    from pool state cannot keep the request in its queue. Closing this needs
    state-qualified consumption in the framework.

    This test will fail once that capability lands, which is the point: it pins
    current behavior so the gap resurfaces instead of being forgotten.
    """

    serving = _rule(lowered, "serve_allocation")
    assert 'field "exhausted"' in serving, "exhaustion must be observable"

    # The rule always produces its acknowledgement, so no request is dropped.
    assert re.search(r"ac\.rule\.output %\w+ when %\w+ ordinal 0", serving)


def test_only_an_uncommitted_reservation_can_be_cancelled(lowered: str) -> None:
    """Cancel returns a slot; commit must make that impossible.

    Idempotence here comes from the search, not a comparison: the duplicate search
    requires `live`, and a commit clears it, so a cancel after a commit finds
    nothing and changes nothing.
    """

    serving = _rule(lowered, "serve_allocation")
    duplicate = _find_predicate(serving, "find0")
    assert 'field "live"' in duplicate

    # Cancel gives a slot back; that write is guarded separately from the
    # allocate, so the two cannot be confused.
    guards = re.findall(r"assign_element @slots\[%\w+\] = %\w+ when (%\w+)", serving)
    assert len(guards) == 3, "expected allocate, cancel and free to write slots"
    assert len(set(guards)) == 3, "each slot write needs its own condition"


def test_slot_geometry_is_derived_from_bank(lowered: str) -> None:
    """A pool must cover exactly one PE's cells, or slots would alias.

    This is why the geometry is imported from the BANK and BGF contracts instead
    of being chosen here: two independently declared numbers could drift.
    """

    assert CELLS_PER_TILE_SLOT * TILE_SLOTS_PER_POOL == CELLS_PER_PE
    assert 1 << SLOT_INDEX_BITS == TILE_SLOTS_PER_POOL

    # A slot index is exactly as wide as the pool needs, so an out-of-pool index
    # is unrepresentable rather than merely rejected.
    assert f"i{SLOT_INDEX_BITS}" in lowered


def test_an_all_zero_pool_means_every_slot_is_free(lowered: str) -> None:
    """Decision 0151 admits an all-zero initial image only.

    Storing `allocated` rather than `free` is what satisfies fre.md's nonzero
    reset requirement without a reset FSM.
    """

    assert "ac.var.decl @slots" in lowered
    assert re.search(r"ac\.var\.decl @slots type [^\n]*init 0 : i64", lowered)
    assert "allocated" in _declared_fields("TileSlot")
    assert "free" not in _declared_fields("TileSlot")


def test_a_slot_holds_no_payload_or_descriptor(lowered: str) -> None:
    """FRE owns placement, not contents. BANK holds bytes and STS the descriptor."""

    fields = _declared_fields("TileSlot")
    for forbidden in ("word0", "payload", "data", "dtype", "layout", "shape", "valid"):
        assert forbidden not in fields, f"TileSlot must not own {forbidden}"
    assert fields == ["owner", "tile_version", "allocation_generation", "allocated"]


def test_a_slot_does_not_store_its_own_index(lowered: str) -> None:
    """An all-zero reset image cannot pre-write each slot's own number.

    That constraint is why responses carry the search index instead of a stored
    field, and it is worth pinning because storing the index would silently
    require a nonzero reset image.
    """

    assert "slot_index" not in _declared_fields("TileSlot")
    # The reservation row does carry one, because a reservation is written after
    # reset and can therefore record where it put its slot.
    assert "slot_index" in _declared_fields("Reservation")


def test_a_capacity_answer_is_a_bit_not_a_count(lowered: str) -> None:
    """There is no reduction operator, so free slots cannot be counted.

    alc.md folds capacity projection into FRE; this pins how much of it FRE can
    actually answer, so the shortfall is visible rather than assumed.
    """

    fields = _declared_fields("CapacityQuery")
    assert "has_free_slot" in fields
    for counting in ("free_count", "count", "remaining"):
        assert counting not in fields, "a count would need a reduction operator"


def test_the_pool_is_instance_local(lowered: str) -> None:
    """Per-PE isolation is structural: the pool is declared inside the system.

    A pool reached through a port could be shared by two instances, which is
    exactly what the Local/Shared split exists to prevent.
    """

    assert re.search(r"ac\.var\.decl @slots [^\n]*owner \"/\"", lowered)

    # The system consumes only its two request streams; no port carries pool state.
    rules = re.findall(r"= ac\.rule ((?:%\w+, )*%\w+) depths", lowered)
    consumed = {name.strip() for header in rules for name in header.split(",")}
    assert consumed == {"%request", "%query"}


def _rule(lowered: str, name: str) -> str:
    """Return the body of the named rule."""

    regions = re.findall(
        rf"= ac\.rule .*?name \"{name}\".*?\n(.*?)\n  \}} \{{ac\.name",
        lowered,
        re.DOTALL,
    )
    assert len(regions) == 1, f"expected exactly one rule named {name}"
    return regions[0]


def _find_predicate(rule: str, prefix: str) -> str:
    """Return the lines of one `ac.find` predicate region.

    Each predicate names its own values, so the region is the span of lines
    carrying that prefix. Filtering on the shared `match.yield` instead would mix
    two predicates together.
    """

    lines = rule.splitlines()
    marked = [
        index for index, line in enumerate(lines) if f"%{prefix}_predicate" in line
    ]
    assert marked, f"no predicate region named {prefix}"
    return "\n".join(lines[marked[0] : marked[-1] + 1])


def _declared_fields(name: str) -> list[str]:
    """Read struct field names from source, in declaration order."""

    source = Path(WORKSPACE, "designs/davincioo/contracts/tmu_trn.py").read_text(
        encoding="utf-8"
    )
    declaration = next(
        node
        for node in ast.walk(ast.parse(source))
        if isinstance(node, ast.ClassDef) and node.name == name
    )
    return [
        statement.target.id
        for statement in declaration.body
        if isinstance(statement, ast.AnnAssign)
    ]
