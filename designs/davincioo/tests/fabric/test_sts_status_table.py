"""STS is the only owner of a version's descriptor, coverage and visibility.

These checks pin the behavioral acceptances in designs/davincioo/tmu/trn/sts.md.
One is pinned as *unmet*: a full status table cannot stall its requester in the
current framework, so `test_a_full_table_is_reported_rather_than_stalled` records
the gap instead of asserting a behavior STS does not have. That is the same
Decision 0210 limit FRE and REF hit, not a new one.

The checks about masks and validation order are structural because the properties
they protect are structural. A single coverage mask, or a publication that wrote
before it validated, would be a different design rather than a miscomputation.
"""

from __future__ import annotations

import ast
import re
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_trn import (
    STATUS_ROWS,
    STS_PUBLISH,
    STS_RESERVE,
    STS_ROLLBACK,
    STS_WRITE,
)
from designs.davincioo.tmu.trn.sts import trn_sts_system

WORKSPACE = str(Path(__file__).resolve().parents[4])


@pytest.fixture(scope="module")
def lowered() -> str:
    return ac.jit(trn_sts_system, workspace=WORKSPACE).lower_acir()


def test_the_table_has_exactly_one_writer(lowered: str) -> None:
    """Decision 0151 permits one write endpoint per table."""

    declarations = re.findall(r"ac\.var\.decl @(\w+) type", lowered)
    assert declarations == ["rows"]
    assert f'stable_id "var/rows" shape [{STATUS_ROWS}]' in lowered

    assert "ac.var.assign_element @rows[" in _rule(lowered, "serve_status_request")

    # The query rule sees the same table and never writes it.
    assert "assign" not in _rule(lowered, "answer_status_query")


def test_the_four_operations_share_one_stream(lowered: str) -> None:
    """Four input ports would deadlock, since a rule needs every input token.

    Decision 0167/0168 fire a rule only when all its inputs have tokens. A
    reserve, a write, a publish and a rollback arrive independently, so they must
    share one stream.
    """

    for name in ("serve_status_request", "answer_status_query"):
        header = re.search(
            rf"= ac\.rule ((?:%\w+, )*%\w+) depths .*name \"{name}\"", lowered
        )
        assert header is not None, f"no rule named {name}"
        assert "," not in header.group(1), f"{name} consumes more than one queue"

    serving = _rule(lowered, "serve_status_request")
    for operation in (STS_RESERVE, STS_WRITE, STS_PUBLISH, STS_ROLLBACK):
        assert f"ac.var.constant {operation} : i8" in serving

    assert re.search(r"= ac\.route %completed depths \[1, 1, 1, 1\]", lowered), (
        "acknowledgements must fan out to one port per operation"
    )


def test_allocation_does_not_imply_definedness(lowered: str) -> None:
    """Reserving a version says where it may be written, never that it was.

    The two masks are separate fields and a reserve installs a zero initialized
    mask. Collapsing them into one mask would make a freshly reserved version read
    as fully defined, which is the exact confusion this acceptance forbids.
    """

    fields = _declared_fields("StatusRow")
    assert "allocation_mask" in fields
    assert "initialized_mask" in fields

    serving = _rule(lowered, "serve_status_request")
    # The only 64-bit zero in the rule is the initialized mask a reserve installs.
    assert "ac.var.constant 0 : i64" in serving


def test_a_write_merges_coverage_instead_of_replacing_it(lowered: str) -> None:
    """Partial writes arrive separately, so a replacing update would erase them."""

    serving = _rule(lowered, "serve_status_request")
    merges = re.findall(r"ac\.var\.or %\w+, %\w+ : !ac\.var<i64>", serving)
    assert len(merges) == 2, "both coverage masks must merge, not replace"


def test_publication_validates_before_anything_changes(lowered: str) -> None:
    """An incompatible descriptor must leave every visible field untouched.

    Compatibility is read from the committed row, so the publish cannot smuggle a
    different descriptor in while making the version visible. Because the check is
    part of the write's condition rather than a later correction, there is no
    interval in which an incompatible version is published.
    """

    serving = _rule(lowered, "serve_status_request")
    row = re.search(r"(%\w+) = ac\.var\.choose", serving) or re.search(
        r"(%\w+) = ac\.var\.match", serving
    )
    assert row is not None, "the row must come from a search over committed state"

    for descriptor in ("dtype", "layout", "shape"):
        assert f'field "{descriptor}"' in serving, (
            f"publication must compare {descriptor}"
        )

    # A refusal is its own outcome, so a caller can tell "incompatible" from
    # "not found" without guessing.
    outcomes = {}
    for field in ("accepted", "matched", "rejected", "exhausted"):
        match = re.search(rf"ac\.var\.with %\w+, (%\w+) field \"{field}\"", serving)
        assert match is not None, f"no field update for {field}"
        outcomes[field] = match.group(1)
    assert len(set(outcomes.values())) == 4, (
        f"each outcome must be computed separately; got {outcomes}"
    )


def test_a_rollback_cannot_erase_a_published_row(lowered: str) -> None:
    """A published version is architectural, so recovery must not reach it.

    The guard requires the row to be speculative and unpublished, and `not x`
    lowers to a comparison against false, so both bits are read in the condition
    rather than assumed by the caller.
    """

    serving = _rule(lowered, "serve_status_request")
    assert 'field "published"' in serving
    assert 'field "speculative"' in serving
    assert re.search(r"ac\.var\.cmp \"eq\" %\w+, %\w+ : !ac\.var<i1>", serving), (
        "the guard must test the committed bits"
    )


def test_contents_defined_is_computed_and_not_stored(lowered: str) -> None:
    """A stored copy would be a second owner of a fact the masks already carry.

    A version whose allocation grew after its last write then reads as
    incompletely defined without anything having to remember to clear a flag.
    """

    assert "contents_defined" not in _declared_fields("StatusRow")
    assert "contents_defined" in _declared_fields("StatusQuery")

    query = _rule(lowered, "answer_status_query")
    assert re.search(r"ac\.var\.cmp \"eq\" %\w+, %\w+ : !ac\.var<i64>", query), (
        "definedness must compare the two masks"
    )


def test_the_first_fault_is_retained(lowered: str) -> None:
    """The fault a caller reads should be the one that caused the others."""

    serving = _rule(lowered, "serve_status_request")
    assert 'field "fault_code"' in serving
    assert re.search(
        r"ac\.var\.select %\w+, %\w+, %\w+ : !ac\.var<i1>, !ac\.var<i8>", serving
    ), "a row that already has a fault must keep it"


def test_a_row_holds_no_payload_bytes(lowered: str) -> None:
    """STS contains no payload bytes; BANK owns those."""

    fields = _declared_fields("StatusRow")
    for forbidden in ("word0", "payload", "data", "cell", "slot_index"):
        assert forbidden not in fields, f"StatusRow must not own {forbidden}"


def test_an_all_zero_table_means_no_version_exists(lowered: str) -> None:
    """Decision 0151 admits an all-zero initial image only.

    Storing `live` rather than a free bit is what makes the reset image mean "no
    version is described here" without a reset pass.
    """

    assert re.search(r"ac\.var\.decl @rows type [^\n]*init 0 : i64", lowered)
    assert "live" in _declared_fields("StatusRow")


def test_a_full_table_is_reported_rather_than_stalled(lowered: str) -> None:
    """UNMET acceptance, recorded rather than asserted.

    sts.md requires that a blocked acknowledgement prevents request consumption
    and writes. That holds for output backpressure, which the inferred transaction
    covers, but not for a reserve that finds no free row: it is reported as
    `exhausted` and consumed.

    Decision 0210 fixes a rule's consumption condition to a proven-constant-true
    candidate, so a condition derived from table occupancy cannot keep the request
    in its queue. This test will fail once state-qualified consumption lands,
    which is the point: it pins current behavior so the gap resurfaces.
    """

    serving = _rule(lowered, "serve_status_request")
    assert 'field "exhausted"' in serving

    condition = re.search(r"ac\.rule\.condition (%\w+)", serving)
    assert condition is not None
    assert f"{condition.group(1)} = ac.var.constant true" in serving


def test_the_table_is_instance_local(lowered: str) -> None:
    """A table reached through a port could be shared by two rename scopes."""

    assert re.search(r"ac\.var\.decl @rows [^\n]*owner \"/\"", lowered)

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
