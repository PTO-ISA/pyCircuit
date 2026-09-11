"""RAT is the only owner of the logical-name-to-version mapping.

These checks pin the behavioral acceptances in designs/davincioo/tmu/trn/rat.md.
One is pinned as *unmet*: a full speculative history cannot stall its requester in
the current framework, so `test_a_full_history_is_reported_rather_than_stalled`
records the gap instead of asserting a behavior RAT does not have. That is the
same Decision 0210 limit FRE and REF hit, not a new one.

Several checks are structural rather than functional because the properties they
protect are structural. A map with two writers, or a history that had to be
searched by age, would break Decision 0151 or lose youngest-first ordering rather
than merely compute the wrong answer, so those are asserted on the lowered module.
"""

from __future__ import annotations

import ast
import re
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_trn import (
    LOGICAL_INDEX_BITS,
    LOGICAL_TILE_REGS,
    MAP_HISTORY_DEPTH,
    RAT_PUBLISH,
    RAT_ROLLBACK,
    RAT_SWAP,
)
from designs.davincioo.tmu.trn.rat import trn_rat_system

WORKSPACE = str(Path(__file__).resolve().parents[4])


@pytest.fixture(scope="module")
def lowered() -> str:
    return ac.jit(trn_rat_system, workspace=WORKSPACE).lower_acir()


def test_the_map_and_history_have_exactly_one_writer(lowered: str) -> None:
    """Decision 0151 permits one write endpoint per owner."""

    declarations = re.findall(r"ac\.var\.decl @(\w+) type", lowered)
    assert declarations == ["history_depth", "map_rows", "history"]
    assert f'stable_id "var/map_rows" shape [{LOGICAL_TILE_REGS}]' in lowered
    assert f'stable_id "var/history" shape [{MAP_HISTORY_DEPTH}]' in lowered

    serving = _rule(lowered, "serve_map_request")
    assert "ac.var.assign_element @map_rows[" in serving
    assert "ac.var.assign_element @history[" in serving
    assert "ac.var.assign @history_depth" in serving

    # The lookup rule sees the same map and never writes it.
    assert "assign" not in _rule(lowered, "answer_rename_lookup")


def test_the_three_operations_share_one_stream(lowered: str) -> None:
    """Three input ports would deadlock, since a rule needs every input token.

    Decision 0167/0168 fire a rule only when all its inputs have tokens. A swap, a
    publish and a rollback arrive independently, so they must share one stream.
    """

    for name in ("serve_map_request", "answer_rename_lookup"):
        header = re.search(
            rf"= ac\.rule ((?:%\w+, )*%\w+) depths .*name \"{name}\"", lowered
        )
        assert header is not None, f"no rule named {name}"
        assert "," not in header.group(1), f"{name} consumes more than one queue"

    serving = _rule(lowered, "serve_map_request")
    for operation in (RAT_SWAP, RAT_PUBLISH, RAT_ROLLBACK):
        assert f"ac.var.constant {operation} : i8" in serving

    # Each acknowledgement still leaves on its own port, via a route.
    assert re.search(r"= ac\.route %completed depths \[1, 1, 1, 1\]", lowered), (
        "acknowledgements must fan out to one port per operation"
    )


def test_an_unknown_operation_has_a_port_instead_of_a_failure(lowered: str) -> None:
    """A selector outside `[0, outputs)` is a deterministic runtime failure.

    Three operations in a two-bit branch leave a fourth value reachable, so it is
    given a port and refused there. Routing it out of range would fail the model
    instead of answering the request, which is the one outcome a request stream
    must never produce.
    """

    route = re.search(r"(%\w+, %\w+, %\w+, %\w+) = ac\.route %completed", lowered)
    assert route is not None
    assert route.group(1).split(", ")[-1] == "%refused"


def test_a_swap_installs_and_records_together(lowered: str) -> None:
    """A mapping overwritten without its history entry could not be rolled back.

    All three operations write one map row, so the lowered module has exactly one
    map write whose guard is the disjunction of the three. What must hold is the
    pairing: the history push carries the swap half of that disjunction, so the
    entry is pushed on the same condition that overwrites the row.

    Two writes of one owner would have to be provably disjoint or exclusive, and
    "a swap and a rollback address different names" is true but not derivable
    from the guards, so a second write would be rejected outright rather than
    merely lose this pairing.
    """

    serving = _rule(lowered, "serve_map_request")
    push = re.search(r"assign_element @history\[%\w+\] = %\w+ when (%\w+)", serving)
    assert push is not None
    swap_guard = push.group(1)

    map_guards = re.findall(
        r"assign_element @map_rows\[%\w+\] = %\w+ when (%\w+)", serving
    )
    assert map_guards == [map_guards[0]], "the map must have exactly one write"

    reached = {map_guards[0]}
    for _ in range(4):
        for guard in list(reached):
            match = re.search(
                rf"{re.escape(guard)} = ac\.var\.or (%\w+), (%\w+)", serving
            )
            if match is not None:
                reached.update(match.groups())
    assert swap_guard in reached, "the map write must be selected by the swap"

    # The stack pointer moves on the same operations that push or pop, never
    # unconditionally: an unconditional scalar write beside conditional array
    # writes has no presence to carry and is rejected.
    depth = re.search(r"ac\.var\.assign @history_depth = %\w+ when (%\w+)", serving)
    assert depth is not None, "the stack pointer write must carry a presence"


def test_a_name_is_an_index_and_not_a_search(lowered: str) -> None:
    """The namespace is dense, so a lookup is an index rather than a match.

    This is worth pinning because searching the map would silently allow two rows
    for one name, which is exactly the duplication LRM's alias disposition forbids.
    """

    serving = _rule(lowered, "serve_map_request")
    assert "ac.var.read_element @map_rows[" in serving
    assert "ac.var.match" not in serving

    # A name is exactly as wide as the namespace, so a name outside it is
    # unrepresentable rather than wrapped onto a live mapping.
    assert 1 << LOGICAL_INDEX_BITS == LOGICAL_TILE_REGS
    assert f"i{LOGICAL_INDEX_BITS}" in lowered


def test_rollback_reads_the_stack_top(lowered: str) -> None:
    """Youngest-first is the shape of the storage, not a property of a search.

    `ac.find`'s key selects the *minimum* key, so age cannot be expressed as a
    search at all. The rollback therefore reads `depth - 1`, masked into the
    history, and that is the only entry it can undo.
    """

    serving = _rule(lowered, "serve_map_request")
    top = re.search(
        r"(%\w+) = ac\.var\.sub %\w+, %\w+ : !ac\.var<i6>\n"
        r"\s+(%\w+) = ac\.var\.constant \d+ : i6[^\n]*\n"
        r"\s+(%\w+) = ac\.var\.and \1, \2",
        serving,
    )
    assert top is not None, "rollback must read depth - 1"
    assert f"ac.var.read_element @history[{top.group(3)}]" in serving


def test_a_rollback_restores_the_complete_old_row(lowered: str) -> None:
    """Restoring only the version would leave a name valid that never was.

    A name with no prior mapping must go back to having none, so the history entry
    carries the validity and publication of the row it displaced, not just its
    version.
    """

    fields = _declared_fields("MapHistory")
    for required in (
        "old_version",
        "old_generation",
        "old_published_version",
        "old_valid",
        "old_published",
    ):
        assert required in fields, f"history entry must retain {required}"


def test_a_rollback_is_generation_qualified(lowered: str) -> None:
    """A stale checkpoint must not restore a mapping newer than it captured."""

    serving = _rule(lowered, "serve_map_request")
    newest = re.search(r"(%\w+) = ac\.var\.read_element @history\[", serving)
    assert newest is not None
    assert f'ac.var.get {newest.group(1)} field "map_generation"' in serving, (
        "the rollback guard must compare the stack top's generation"
    )


def test_an_all_zero_map_means_no_name_has_a_version(lowered: str) -> None:
    """Decision 0151 admits an all-zero initial image only.

    rat.md asks for a nonzero architectural reset image. Storing `valid` is how
    that requirement is met without a reset pass: a reset name reads as "no
    version yet" rather than as a mapping to version zero.
    """

    assert re.search(r"ac\.var\.decl @map_rows type [^\n]*init 0 : i64", lowered)
    assert re.search(r"ac\.var\.decl @history_depth type i6 init 0", lowered)
    assert "valid" in _declared_fields("MapRow")


def test_a_map_row_holds_no_payload_or_descriptor(lowered: str) -> None:
    """RAT owns names, not contents. BANK holds bytes and STS the descriptor."""

    fields = _declared_fields("MapRow")
    for forbidden in ("word0", "payload", "dtype", "layout", "shape", "slot_index"):
        assert forbidden not in fields, f"MapRow must not own {forbidden}"
    assert fields == [
        "current_version",
        "map_generation",
        "published_version",
        "valid",
        "published",
    ]


def test_a_full_history_is_reported_rather_than_stalled(lowered: str) -> None:
    """UNMET acceptance, recorded rather than asserted.

    rat.md requires that a full speculative history stalls before FRE allocation
    commits. RAT reports `history_full` and refuses, which invents no fault and
    drops nothing, but it does consume the request instead of holding it.

    Decision 0210 fixes a rule's consumption condition to a proven-constant-true
    candidate and states the false path may consume input, so a condition derived
    from history depth cannot keep the request in its queue. Closing this needs
    state-qualified consumption in the framework.

    This test will fail once that capability lands, which is the point: it pins
    current behavior so the gap resurfaces instead of being forgotten.
    """

    serving = _rule(lowered, "serve_map_request")
    assert 'field "history_full"' in serving, "a full history must be observable"

    condition = re.search(r"ac\.rule\.condition (%\w+)", serving)
    assert condition is not None
    assert f"{condition.group(1)} = ac.var.constant true" in serving, (
        "consumption is unconditional, which is exactly the gap"
    )


def test_the_map_is_instance_local(lowered: str) -> None:
    """Per-PE isolation is structural: the map is declared inside the system.

    A map reached through a port could be shared by two instances, which is
    exactly what the Local/Shared split exists to prevent.
    """

    assert re.search(r"ac\.var\.decl @map_rows [^\n]*owner \"/\"", lowered)

    rules = re.findall(r"= ac\.rule ((?:%\w+, )*%\w+) depths", lowered)
    consumed = {name.strip() for header in rules for name in header.split(",")}
    assert consumed == {"%request", "%lookup"}


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
