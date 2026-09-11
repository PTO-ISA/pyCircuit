"""BANK stores whole cells, and a stale generation cannot mutate a reused row.

These checks pin what designs/davincioo/tmu/trf/bank.md freezes: 256 rows, one
entry per 128-byte cell, a read latency of 2 at BANK's output, one access per
cycle, and a generation check that gates every write.

The granularity assertions are the point of this file.  Clients address cells and
never parts of cells, so the storage must have no addressable sub-cell
coordinate; an earlier revision stored 64-bit words and exposed a `word` index
that no architectural producer owned.
"""

from __future__ import annotations

import ast
import re
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_trf import (
    ACCESS_INVALIDATE,
    ACCESS_WRITE,
    CELL_BYTES,
    CELL_OUTPUT_STAGES,
    CELL_READ_LATENCY,
    ROW_INDEX_MASK,
    ROWS_PER_BANK,
    RULE_ACCESS_LATENCY,
    WORDS_PER_CELL,
)
from designs.davincioo.tmu.trf.bank import trf_bank_system

WORKSPACE = "/home/xiekunpeng/linxISA/pyCircuit"


@pytest.fixture(scope="module")
def lowered() -> str:
    return ac.jit(trf_bank_system, workspace=WORKSPACE).lower_acir()


def test_one_entry_holds_one_whole_cell(lowered: str) -> None:
    """The stored element is a whole cell, so storage granularity matches access.

    The layout spec is the strongest available evidence: it reports the element's
    real size, which must be exactly one cell.
    """

    assert (
        f"!ac.struct<@types::@CellData> = {{abi_alignment = 8 : i64, "
        f'endianness = "little", preferred_alignment = 8 : i64, '
        f"size = {CELL_BYTES} : i64}}" in lowered
    )

    # One entry per row, and the entry type is the cell, not a fragment of one.
    assert (
        "ac.var.decl @cells type !ac.struct<@types::@CellData> init 0 : i64 "
        f'owner "/" stable_id "var/cells" shape [{ROWS_PER_BANK}]' in lowered
    )
    assert len(_struct_fields(lowered, "CellData")) == WORDS_PER_CELL


def test_no_addressable_sub_cell_coordinate(lowered: str) -> None:
    """`row` is the only coordinate; the sixteen words are not selectable.

    The words exist only because 64 bits is the widest admissible integer.  If
    any index arithmetic combined `row` with a second coordinate, some interface
    could reach a fragment of a cell, which the architecture does not permit.
    """

    fields = _struct_fields(lowered, "BankAccess")
    assert "word" not in fields
    assert {f"word{index}" for index in range(WORDS_PER_CELL)} <= set(fields)

    rule = _rule_region(lowered)

    # Both arrays are indexed by one masked row and nothing else.
    indices = set(re.findall(r"ac\.var\.read_element @\w+\[(%\w+)\]", rule))
    assert len(indices) == 1
    index = indices.pop()
    assert re.search(rf"{index} = ac\.var\.and %\w+, %\w+", rule)

    # No shift/or address composition, which is how a (row, word) pair was built.
    assert "ac.var.shl" not in rule
    assert "ac.var.or " not in rule

    # Every write targets that same index, so nothing addresses a sub-cell.
    for target in re.findall(r"ac\.var\.assign_element @\w+\[(%\w+)\]", rule):
        assert target == index


def test_row_is_masked_before_it_indexes_either_array(lowered: str) -> None:
    """The bound is structural, not a promise from upstream.

    designs/davincioo/tmu/bgf/map.py reports an out-of-range result but never
    enforces it, so BANK must not be reachable past its last row.

    What this does not establish is isolation. Masking bounds the index but aliases
    rather than refuses: a row past ROWS_PER_BANK wraps onto a different valid row
    of this bank, so an out-of-range access reads or writes another cell instead of
    failing. bank.md records the absent refusal owner as an open decision.
    """

    rule = _rule_region(lowered)
    match = re.search(
        r'ac\.var\.get %item field "row" : [^\n]*\n'
        r"\s*(%\w+) = ac\.var\.constant (\d+) : i16",
        rule,
    )
    assert match is not None
    assert int(match.group(2)) == ROW_INDEX_MASK
    assert ROW_INDEX_MASK == ROWS_PER_BANK - 1


def test_a_single_rule_makes_the_bank_single_port(lowered: str) -> None:
    """One rule admits one token per tick, so one access reaches the arrays."""

    assert lowered.count(" = ac.rule ") == 1

    # Both arrays live behind that one rule, so they cannot be reached in
    # parallel by separate endpoints.
    declarations = re.findall(r"ac\.var\.decl @(\w+) type", lowered)
    assert declarations == ["cells", "generations"]

    # Exactly one writer per array, which Decision 0151 requires.
    rule = _rule_region(lowered)
    writers = re.findall(r"ac\.var\.assign_element @(\w+)\[", rule)
    assert sorted(writers) == ["cells", "generations"]


def test_read_latency_at_the_output_is_exactly_two(lowered: str) -> None:
    """The rule stage plus the output stage must sum to the frozen latency."""

    assert re.search(
        rf"= ac\.rule %access depths \[1\] latencies \[{RULE_ACCESS_LATENCY}\]",
        lowered,
    )
    assert re.search(
        rf"= ac\.transform %served depths \[1\] latencies \[{CELL_OUTPUT_STAGES}\]",
        lowered,
    )
    assert RULE_ACCESS_LATENCY + CELL_OUTPUT_STAGES == CELL_READ_LATENCY


def test_no_write_commits_without_a_current_generation(lowered: str) -> None:
    rule = _rule_region(lowered)

    # The payload write is conditional, and its condition is not a constant.
    match = re.search(
        r"ac\.var\.assign_element @cells\[%\w+\] = %\w+ when (%\w+)", rule
    )
    assert match is not None
    condition = match.group(1)
    assert f"{condition} = ac.var.constant true" not in rule

    # That condition conjoins a generation match with a write kind, so a stale
    # write degenerates into a read of the same row.
    definition = re.search(rf"{condition} = ac\.var\.(and|mul) (%\w+), (%\w+)", rule)
    assert definition is not None
    generation_match, kind_match = definition.group(2), definition.group(3)
    assert _defines_generation_comparison(rule, generation_match)
    assert _defines_kind_comparison(rule, kind_match, ACCESS_WRITE)

    # Both decisions are reported, so a rejected write is observable.
    assert 'ac.var.with %item, %v3 field "stored_generation"' in rule
    assert 'field "current"' in rule
    assert 'field "applied"' in rule


def test_invalidate_installs_a_generation_without_touching_bytes(
    lowered: str,
) -> None:
    rule = _rule_region(lowered)

    match = re.search(
        r"ac\.var\.assign_element @generations\[%\w+\] = (%\w+) when (%\w+)", rule
    )
    assert match is not None
    value, condition = match.group(1), match.group(2)

    # It installs the requested generation, gated on the invalidate kind alone.
    assert f'{value} = ac.var.get %item field "installed_generation"' in rule
    assert _defines_kind_comparison(rule, condition, ACCESS_INVALIDATE)

    # An invalidation does not clear the cell: the only cells write is the
    # generation-qualified payload write, which requires the write kind.
    assert rule.count("ac.var.assign_element @cells[") == 1


def test_the_request_is_never_dropped(lowered: str) -> None:
    """The requester is waiting for a response whatever the outcome."""

    rule = _rule_region(lowered)
    match = re.search(r"ac\.rule\.output %\w+ when (%\w+) ordinal 0", rule)
    assert match is not None
    assert f"{match.group(1)} = ac.var.constant true" in rule


def test_the_stored_cell_carries_no_request_identity(lowered: str) -> None:
    """Storing identity would misroute a later read's response.

    A cell outlives the request that wrote it, so if identity were stored, a read
    would return the previous writer's identity rather than its own requester's.
    """

    stored = _struct_fields(lowered, "CellData")
    identity = (
        "requester",
        "request_id",
        "response_route",
        "flow_id",
        "thread_id",
        "sequence",
        "tile_id",
        "tile_version",
    )
    assert not [name for name in identity if name in stored]

    # Identity in the response comes from the request record itself.
    rule = _rule_region(lowered)
    assert 'ac.var.with %item, %v3 field "stored_generation"' in rule


def test_bank_owns_no_descriptor_or_definedness_field(lowered: str) -> None:
    fields = _struct_fields(lowered, "BankAccess")

    # bank.md excludes descriptor state from raw payload ownership.
    forbidden = ("shape", "dtype", "layout", "valid_region", "defined")
    assert not [name for name in fields if any(word in name for word in forbidden)]
    # `row` is the sole placement input.
    assert "row" in fields
    assert not [name for name in fields if name in ("column", "offset", "byte_mask")]


def test_cell_key_never_substitutes_for_tile_version(lowered: str) -> None:
    """CELL's schema acceptance, proved by absence.

    designs/davincioo/tmu/trf/cell.md requires that CellKey never substitutes for
    TileVersion.  BANK satisfies it structurally: it never reads `cell_key` at
    all.  Addressing uses `row`, and the generation decision uses `tile_version`,
    so identity and location cannot be confused even by mistake.
    """

    assert 'field "cell_key"' not in lowered

    rule = _rule_region(lowered)
    assert 'ac.var.get %item field "tile_version"' in rule
    assert "ac.var.read_element @generations" in rule


def test_cell_schema_lives_in_bank_owned_arrays_only() -> None:
    """CELL is a state schema, so no independent leaf may own it.

    cell.md authorises no independent leaf and no separate Queue/state owner, so
    a `cell.py` module would create a second owner of the same bytes.  The schema
    is the geometry in the TRF contract plus the two arrays BANK elaborates.
    """

    assert not Path(WORKSPACE, "designs/davincioo/tmu/trf/cell.py").exists()

    from designs.davincioo.contracts import tmu_trf

    assert tmu_trf.WORDS_PER_CELL * 64 == tmu_trf.CELL_BYTES * 8
    assert tmu_trf.ROW_INDEX_MASK == tmu_trf.ROWS_PER_BANK - 1
    assert tmu_trf.CELL_OUTPUT_STAGES >= 1

    # The declared fields must match the frozen geometry, since the record is
    # what actually carries a cell.
    declared = _declared_fields("BankAccess")
    assert [name for name in declared if name.startswith("word")] == [
        f"word{index}" for index in range(tmu_trf.WORDS_PER_CELL)
    ]
    assert set(_declared_fields("CellData")) == {
        f"word{index}" for index in range(tmu_trf.WORDS_PER_CELL)
    }


def test_access_kinds_are_closed_and_distinct() -> None:
    from designs.davincioo.contracts.tmu_trf import ACCESS_READ

    kinds = (ACCESS_READ, ACCESS_WRITE, ACCESS_INVALIDATE)
    assert len(set(kinds)) == len(kinds)
    # A read is the default, so a zeroed record cannot accidentally mutate state.
    assert ACCESS_READ == 0


def _struct_fields(lowered: str, name: str) -> dict[str, str]:
    """Return the lowered field names and types of one struct."""

    match = re.search(rf"ac\.struct @{name} fields \[(.*?)\]\n", lowered)
    assert match is not None, f"struct {name} is absent from the lowered module"
    return dict(re.findall(r'\{name = "(\w+)", type = (\w+)\}', match.group(1)))


def _declared_fields(name: str) -> list[str]:
    """Read field names from source, in declaration order.

    ``@ac.struct`` does not expose field metadata at runtime, so the declaration
    is the only place this can be checked.
    """

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


def _rule_region(lowered: str) -> str:
    """Return the body of the sole rule."""

    regions = re.findall(r"= ac\.rule .*?\n(.*?)\n  \} \{ac\.name", lowered, re.DOTALL)
    assert len(regions) == 1
    return regions[0]


def _defines_generation_comparison(rule: str, value: str) -> bool:
    """True when `value` is the stored-generation/TileVersion equality."""

    match = re.search(rf"{value} = ac\.var\.cmp \"eq\" (%\w+), (%\w+)", rule)
    if match is None:
        return False
    left, right = match.group(1), match.group(2)
    return (
        f"{left} = ac.var.read_element @generations" in rule
        and f'{right} = ac.var.get %item field "tile_version"' in rule
    )


def _defines_kind_comparison(rule: str, value: str, kind: int) -> bool:
    """True when `value` is the equality between `kind` and the given tag."""

    match = re.search(rf"{value} = ac\.var\.cmp \"eq\" (%\w+), (%\w+)", rule)
    if match is None:
        return False
    left, right = match.group(1), match.group(2)
    return (
        f'{left} = ac.var.get %item field "kind"' in rule
        and f"{right} = ac.var.constant {kind} : i8" in rule
    )
