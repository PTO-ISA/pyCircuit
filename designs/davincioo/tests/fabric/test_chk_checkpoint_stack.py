"""CHK owns captured recovery points and nothing else.

These checks pin the behavioral acceptances in designs/davincioo/tmu/trn/chk.md.
Two properties are recorded rather than proven here, because they are not CHK's
to prove: youngest-first ordering belongs to RAT's history stack, and step
sequencing has no owner in the current framework. `test_an_unwind_reports_a
_distance_rather_than_driving_it` pins that boundary so it stays visible.

The checks are structural because the properties they protect are structural. A
CHK that wrote a sibling's state, or that stored a copy of the map, would be a
different module rather than a miscomputing one.
"""

from __future__ import annotations

import ast
import re
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_trn import (
    CHECKPOINT_SLOTS,
    CHK_CAPTURE,
    CHK_RELEASE,
    CHK_UNWIND,
)
from designs.davincioo.tmu.trn.chk import trn_chk_system

WORKSPACE = str(Path(__file__).resolve().parents[4])


@pytest.fixture(scope="module")
def lowered() -> str:
    return ac.jit(trn_chk_system, workspace=WORKSPACE).lower_acir()


def test_chk_owns_checkpoints_and_nothing_else(lowered: str) -> None:
    """No direct mutation of sibling RAT/FRE/STS state.

    The strongest form of that acceptance is that no sibling state is reachable
    at all: the module declares one owner and consumes one request stream.
    """

    declarations = re.findall(r"ac\.var\.decl @(\w+) type", lowered)
    assert declarations == ["checkpoints"]
    assert f'stable_id "var/checkpoints" shape [{CHECKPOINT_SLOTS}]' in lowered
    assert re.search(r"ac\.var\.decl @checkpoints [^\n]*owner \"/\"", lowered)

    rules = re.findall(r"= ac\.rule ((?:%\w+, )*%\w+) depths", lowered)
    consumed = {name.strip() for header in rules for name in header.split(",")}
    assert consumed == {"%request"}


def test_a_checkpoint_is_a_frontier_and_not_a_copy_of_the_map(lowered: str) -> None:
    """Copying displaced mappings here would make two owners of one fact.

    RAT already retains every displaced mapping, so a checkpoint needs only the
    depth of that history. Storing rows instead would also bound recovery by CHK's
    capacity rather than by RAT's.
    """

    assert _declared_fields("Checkpoint") == [
        "flow_key",
        "checkpoint_id",
        "frontier",
        "live",
    ]


def test_the_three_operations_share_one_stream(lowered: str) -> None:
    """Three input ports would deadlock, since a rule needs every input token."""

    header = re.search(
        r"= ac\.rule ((?:%\w+, )*%\w+) depths .*name \"serve_checkpoint_request\"",
        lowered,
    )
    assert header is not None
    assert "," not in header.group(1)

    serving = _rule(lowered, "serve_checkpoint_request")
    for operation in (CHK_CAPTURE, CHK_UNWIND, CHK_RELEASE):
        assert f"ac.var.constant {operation} : i8" in serving

    route = re.search(r"(%\w+, %\w+, %\w+, %\w+) = ac\.route %completed", lowered)
    assert route is not None, "each answer must leave on its own port"
    # A fourth branch value is reachable in a two-bit branch, so it is refused on
    # a port rather than routed out of range.
    assert route.group(1).split(", ")[-1] == "%refused"


def test_an_unwind_reports_a_distance_rather_than_driving_it(lowered: str) -> None:
    """Recorded boundary, not a proven acceptance.

    chk.md's "recovery unwinds youngest-first" is RAT's property: CHK answers with
    the target frontier and the number of rollbacks that reach it, and RAT pops its
    own stack top for each one. So the unwind writes no CHK state at all, which is
    what this asserts: only a capture and a release touch the table.

    Owning the stepping would need one owner that both registers an unwind from a
    request and advances a cursor without one. A rule fires either on a token or on
    state, not both, and splitting it across two rules would give the cursor two
    writers, which Decision 0151 forbids.
    """

    serving = _rule(lowered, "serve_checkpoint_request")
    guards = re.findall(
        r"assign_element @checkpoints\[%\w+\] = %\w+ when (%\w+)", serving
    )
    assert len(guards) == 2, "only a capture and a release may write the table"
    assert len(set(guards)) == 2, "each write needs its own condition"

    steps = re.search(
        r"(%\w+) = ac\.var\.select (%\w+), %\w+, %\w+ : !ac\.var<i1>, !ac\.var<i6>"
        r" -> !ac\.var<i6>\n\s+%\w+ = ac\.var\.with %\w+, \1 field \"steps\"",
        serving,
    )
    assert steps is not None, "the answer must carry a step distance"
    assert steps.group(2) not in guards, "an unwind must not write the table"


def test_staleness_is_one_comparison_and_not_a_sweep(lowered: str) -> None:
    """A stale checkpoint cannot restore a newer mapping.

    A checkpoint captured after the recovery point has the deeper frontier, so it
    fails `current_frontier >= frontier` on any later use. Nothing sweeps the
    table, which matters because a search writes one row and this storage has no
    masked bulk update: a design that needed the sweep would not be expressible.
    """

    serving = _rule(lowered, "serve_checkpoint_request")
    assert re.search(r"ac\.var\.cmp \"uge\" %\w+, %\w+ : !ac\.var<i6>", serving), (
        "reachability must compare frontiers"
    )
    assert 'field "stale"' in serving


def test_a_repeated_capture_reuses_its_record(lowered: str) -> None:
    """A duplicate checkpoint id must not consume a second row."""

    serving = _rule(lowered, "serve_checkpoint_request")
    duplicate = _find_predicate(serving, "find0")
    assert 'field "live"' in duplicate
    assert 'field "flow_key"' in duplicate
    assert 'field "checkpoint_id"' in duplicate


def test_a_release_is_what_bounds_the_pool(lowered: str) -> None:
    """Without a release, a correctly predicted program would exhaust the pool.

    Recovery is the rare path, so the common path has to give records back; that
    is why release is an operation rather than an implicit effect of an unwind.
    """

    serving = _rule(lowered, "serve_checkpoint_request")
    assert re.search(r"ac\.var\.with %\w+, %\w+ field \"live\"", serving), (
        "a release must retire its record"
    )
    assert 'field "exhausted"' in serving


def test_an_all_zero_pool_means_no_checkpoint_exists(lowered: str) -> None:
    """Decision 0151 admits an all-zero initial image only."""

    assert re.search(r"ac\.var\.decl @checkpoints type [^\n]*init 0 : i64", lowered)
    assert "live" in _declared_fields("Checkpoint")


def test_a_full_pool_is_reported_rather_than_stalled(lowered: str) -> None:
    """UNMET acceptance, recorded rather than asserted.

    A capture that finds no free row is reported as `exhausted` and consumed,
    rather than held until a record is released. Decision 0210 fixes a rule's
    consumption condition to a proven-constant-true candidate, so a condition
    derived from pool occupancy cannot keep the request in its queue. This is the
    same gap FRE, REF and RAT record, and this test will fail once
    state-qualified consumption lands.
    """

    serving = _rule(lowered, "serve_checkpoint_request")
    condition = re.search(r"ac\.rule\.condition (%\w+)", serving)
    assert condition is not None
    assert f"{condition.group(1)} = ac.var.constant true" in serving


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
    """Return the lines of one `ac.find` predicate region."""

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
