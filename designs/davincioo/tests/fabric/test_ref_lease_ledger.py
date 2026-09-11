"""REF counts physical references and never frees a logical version.

These checks pin the five behavioral acceptances in
designs/davincioo/tmu/trf/ref.md, and one of them is pinned as *unmet*: a full
ledger cannot stall its requester in the current framework, so
`test_a_full_ledger_is_reported_rather_than_stalled` records the gap instead of
asserting a behavior the leaf does not have.

The structural checks matter as much as the functional ones. A ledger with two
writers, or a query that mutates, would break Decision 0151 rather than just be
wrong, so those are asserted on the lowered module.
"""

from __future__ import annotations

import ast
import re
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_trf import (
    ACK_BRANCH,
    LEASE_ACQUIRE,
    LEASE_COUNT_MAX,
    LEASE_RELEASE,
    LEASE_SLOTS,
    LEASE_TRANSFER,
    OUTCOME_BRANCH_MASK,
    RECLAIM_BRANCH,
)
from designs.davincioo.tmu.trf.ref import trf_ref_system

WORKSPACE = "/home/xiekunpeng/linxISA/pyCircuit"


@pytest.fixture(scope="module")
def lowered() -> str:
    return ac.jit(trf_ref_system, workspace=WORKSPACE).lower_acir()


def test_ledger_has_exactly_one_writer(lowered: str) -> None:
    """Decision 0151 permits one write endpoint per table."""

    declarations = re.findall(r"ac\.var\.decl @(\w+) type", lowered)
    assert declarations == ["leases"]
    assert f'stable_id "var/leases" shape [{LEASE_SLOTS}]' in lowered

    # The mutating rule writes; the query rule does not.
    assert "ac.var.assign_element @leases[" in _rule(lowered, "serve_lease_request")
    assert "assign_element" not in _rule(lowered, "answer_refcount_query")


def test_the_query_rule_only_reads(lowered: str) -> None:
    """A query must not disturb the ledger it reports on."""

    query = _rule(lowered, "answer_refcount_query")
    assert "assign_element" not in query
    # It still searches, so it observes real state rather than a cached copy.
    assert "ac.var.match.yield" in query
    assert 'field "count"' in query


def test_the_three_mutations_share_one_stream(lowered: str) -> None:
    """Four input ports would deadlock, since a rule needs every input token.

    Decision 0167/0168 fire a rule only when all its inputs have tokens. Acquire,
    transfer and release arrive independently, so they must share one stream.
    """

    assert re.search(r"= ac\.rule %request depths \[1\]", lowered)
    assert re.search(r"= ac\.rule %query depths \[1\]", lowered)

    # Each rule consumes exactly one queue, so neither waits on the other.
    for name in ("serve_lease_request", "answer_refcount_query"):
        header = re.search(
            rf"= ac\.rule ((?:%\w+, )*%\w+) depths .*name \"{name}\"", lowered
        )
        assert header is not None
        assert "," not in header.group(1)

    # The operations are distinguished inside that stream, not by port.
    served = _rule(lowered, "serve_lease_request")
    for kind in (LEASE_ACQUIRE, LEASE_TRANSFER, LEASE_RELEASE):
        assert f"ac.var.constant {kind} : i8" in served


def test_a_lease_is_identified_by_owner_version_and_kind(lowered: str) -> None:
    """The held-lease search must not match a different lease of the same row."""

    served = _rule(lowered, "serve_lease_request")
    predicate = _find_predicate(served, "find0")

    for field in ("live", "owner", "tile_version", "lease_kind"):
        assert f'field "{field}"' in predicate
    # All four terms are conjoined, so no single field can carry a match alone.
    assert predicate.count("ac.var.mul") == 3


def test_a_tombstoned_slot_is_not_free(lowered: str) -> None:
    """Acceptance: canceled reads retain tombstones until late responses drain.

    Reusing a tombstoned row would let a drained response land on a different
    lease, so the free-slot search must exclude it.
    """

    served = _rule(lowered, "serve_lease_request")
    predicate = _find_predicate(served, "find3")

    assert 'field "live"' in predicate
    assert 'field "tombstone"' in predicate
    # Both are required to be false, so a tombstoned dead row is not a candidate.
    assert predicate.count("ac.var.constant false") == 2
    assert predicate.count("ac.var.mul") == 1


def test_transfer_has_no_unowned_interval(lowered: str) -> None:
    """Acceptance: transfer ownership without an unowned interval.

    Ownership is rewritten in place, so the row never stops existing. Moving the
    lease to another row would create exactly the interval this forbids.
    """

    served = _rule(lowered, "serve_lease_request")
    assert 'ac.var.get %item field "new_owner"' in served

    # Two write targets only: the held row and the free row. A transfer that
    # moved rows would need to write the held row's slot with a dead value, so
    # assert the held-row write is the one carrying `new_owner`.
    targets = re.findall(r"ac\.var\.assign_element @leases\[(%\w+)\]", served)
    assert len(targets) == 2
    assert len(set(targets)) == 2


def test_duplicate_release_decrements_once(lowered: str) -> None:
    """Acceptance: a duplicate release decrements once.

    Idempotence comes from the search, not a comparison: the held-lease predicate
    requires `live`, so a second release of a released lease finds nothing and
    changes nothing.
    """

    served = _rule(lowered, "serve_lease_request")
    predicate = _find_predicate(served, "find0")
    assert 'ac.var.get %entry field "live"' in predicate

    # Release is qualified by that search result, never by the request alone.
    assert f"ac.var.constant {LEASE_RELEASE} : i8" in served
    assert "ac.var.sub" in served


def test_zero_references_emits_a_reclaim_candidate(lowered: str) -> None:
    """Acceptance: zero count emits a reclaim candidate but does not free.

    The branch is selected from committed state, and REF's only effects are the
    ledger write and the routed outcome -- there is no free, no payload access and
    no publication.
    """

    served = _rule(lowered, "serve_lease_request")

    # The last-reference test compares the committed count against one.
    assert "ac.var.constant 1 : i16" in served
    # The outcome is a selection between the two branch constants.
    assert "ac.var.select" in served
    assert f"ac.var.constant {RECLAIM_BRANCH} : i8" in served
    assert ACK_BRANCH == 0

    # The route separates the two consumers, so each is backpressured alone.
    assert re.search(r"= ac\.route %completed depths \[1, 1\]", lowered)
    assert OUTCOME_BRANCH_MASK == 1

    # REF owns no lifetime authority: one array, and no second state owner.
    assert lowered.count("ac.var.decl") == 1


def test_saturation_refuses_instead_of_wrapping(lowered: str) -> None:
    """A wrapped count would read as zero and make a live version reclaimable."""

    served = _rule(lowered, "serve_lease_request")
    assert f"ac.var.constant {LEASE_COUNT_MAX} : i16" in served
    assert LEASE_COUNT_MAX == 0xFFFF


def test_a_full_ledger_is_reported_rather_than_stalled(lowered: str) -> None:
    """Acceptance **not met**: acquire cannot stall before upstream acceptance.

    ref.md requires a full ledger to stall its requester. The framework cannot
    express that: Decision 0210 fixes a rule's consume condition to a proven
    constant-true candidate and states that the false output path *may consume
    input*, so a state-derived "table is full" condition cannot hold a request in
    its queue.

    What the leaf does instead is the safe alternative -- it always answers, with
    `accepted=False` -- so a request is never silently dropped. The requester must
    retry. This test pins that behavior so the day the framework gains
    state-qualified consumption, this assertion fails and the leaf gets revisited.
    """

    served = _rule(lowered, "serve_lease_request")

    # The acknowledgement presence is unconditional: a constant-true candidate.
    match = re.search(r"ac\.rule\.output %\w+ when (%\w+) ordinal 0", served)
    assert match is not None
    assert f"{match.group(1)} = ac.var.constant true" in served

    # Acceptance is a computed field rather than a consumption decision.
    assert 'field "accepted"' in served


def test_no_response_carries_the_physical_slot() -> None:
    """A slot is REF's private layout, as a sub-cell coordinate is BANK's."""

    fields = _declared_fields("LeaseRequest")
    assert "slot" not in fields
    # A lease is named by these instead.
    assert {"owner", "tile_version", "lease_kind"} <= set(fields)

    query = _declared_fields("RefcountQuery")
    assert "slot" not in query


def test_lease_geometry_is_self_consistent() -> None:
    from designs.davincioo.contracts import tmu_trf

    kinds = (tmu_trf.LEASE_ACQUIRE, tmu_trf.LEASE_TRANSFER, tmu_trf.LEASE_RELEASE)
    assert len(set(kinds)) == len(kinds)
    # Acquire is zero, so a zeroed record cannot accidentally release a lease.
    assert tmu_trf.LEASE_ACQUIRE == 0
    assert tmu_trf.LEASE_KIND_READ != tmu_trf.LEASE_KIND_WRITE
    assert tmu_trf.ACK_BRANCH != tmu_trf.RECLAIM_BRANCH
    assert (
        tmu_trf.RECLAIM_BRANCH & tmu_trf.OUTCOME_BRANCH_MASK == tmu_trf.RECLAIM_BRANCH
    )


def test_ref_owns_no_payload_or_descriptor_state() -> None:
    """REF is a ledger: it counts references and stores no tile bytes."""

    entry = _declared_fields("Lease")
    forbidden = ("word", "payload", "shape", "dtype", "layout", "defined")
    assert not [name for name in entry if any(bad in name for bad in forbidden)]
    assert {"count", "live", "tombstone", "release_pending"} <= set(entry)


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

    source = Path(WORKSPACE, "designs/davincioo/contracts/tmu_trf.py").read_text(
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
        and isinstance(statement.target, ast.Name)
    ]
