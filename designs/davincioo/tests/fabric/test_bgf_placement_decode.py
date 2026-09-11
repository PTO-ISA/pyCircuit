"""MAP owns the BGF placement decode; RQ and WQ only own residency.

These checks pin the ownership split that designs/davincioo/tmu/bgf/map.md
records: clients address cells, MAP turns ``cell_key`` into ``bank``/``row``
and a bounds result, RQ and WQ carry that result without reading it, and ARB
is the single consumer that turns ``bank`` into a scheduling decision.
"""

from __future__ import annotations

import ast
import re
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_bgf import (
    BANK_INDEX_BITS,
    BANK_INDEX_MASK,
    BANK_PARTITIONS,
    CLASS_RESIDENCY_DEPTH,
    SOURCE_CLASS_COUNT,
)
from designs.davincioo.contracts.tmu_trf import ROWS_PER_BANK
from designs.davincioo.tmu.bgf.arb import bgf_arb_system
from designs.davincioo.tmu.bgf.map import bgf_map_system
from designs.davincioo.tmu.bgf.rq import bgf_rq_system
from designs.davincioo.tmu.bgf.wq import bgf_wq_system

ROOT = Path(__file__).resolve().parents[4]
BGF = ROOT / "designs/davincioo/tmu/bgf"
PLACEMENT_FIELDS = ("source_class", "bank", "row", "out_of_range")


def _lowered(system: object) -> str:
    return ac.jit(system, workspace=ROOT).lower_acir()


def _transform_bodies(raw: str) -> dict[str, str]:
    bodies: dict[str, str] = {}
    name: str | None = None
    for line in raw.splitlines():
        header = re.match(r"\s*%(\S+) = ac\.transform ", line)
        if header is not None:
            name = header.group(1)
            bodies[name] = ""
        elif name is not None:
            if line.startswith("  }"):
                name = None
            else:
                bodies[name] += line + "\n"
    return bodies


def test_map_decodes_every_lane_from_cell_key() -> None:
    bodies = _transform_bodies(_lowered(bgf_map_system))
    assert len(bodies) == 2 * SOURCE_CLASS_COUNT

    for direction in ("mapped_reads", "mapped_writes"):
        for lane in range(SOURCE_CLASS_COUNT):
            body = bodies[f"{direction}__{lane}"]
            # The lane ordinal is the source-class tag, written as a constant.
            assert f"ac.var.constant {lane} : i8" in body
            assert 'field "source_class"' in body
            # bank = cell_key & BANK_INDEX_MASK, row = cell_key >> BANK_INDEX_BITS
            assert f"ac.var.constant {BANK_INDEX_MASK} : i16" in body
            assert "ac.var.and" in body
            assert f"ac.var.constant {BANK_INDEX_BITS} : i16" in body
            assert "ac.var.shr" in body
            # The bounds result is reported, never used to drop the request.
            assert f"ac.var.constant {ROWS_PER_BANK} : i16" in body
            assert 'ac.var.cmp "uge"' in body
            assert 'field "out_of_range"' in body
            # Identity is appended to, not replaced.
            assert body.count("ac.var.with") == len(PLACEMENT_FIELDS)


@pytest.mark.parametrize(
    ("system", "lanes"),
    ((bgf_rq_system, "read"), (bgf_wq_system, "write")),
    ids=("rq", "wq"),
)
def test_queue_schemas_are_bank_blind_residency(system: object, lanes: str) -> None:
    raw = _lowered(system)

    # No bank dimension: one queue per class, and no fan-out of any kind.
    assert " = ac.route " not in raw
    assert " = ac.merge " not in raw
    assert " = ac.fork " not in raw
    assert raw.count(" = ac.source ") == SOURCE_CLASS_COUNT

    # No field of the record is read, so no placement result can be acted on.
    assert "ac.var.get" not in raw

    # Residency is the whole content: identity transforms carrying the depth.
    bodies = _transform_bodies(raw)
    assert len(bodies) == SOURCE_CLASS_COUNT
    for body in bodies.values():
        assert body.strip().splitlines()[-1].startswith("    ac.transform.yield %item")
        assert "ac.var." not in body
    transforms = [line for line in raw.splitlines() if " = ac.transform " in line]
    assert all(
        f"depths [{CLASS_RESIDENCY_DEPTH}] latencies [1]" in line for line in transforms
    )
    assert lanes in raw


def test_arb_is_the_only_consumer_of_the_decoded_bank() -> None:
    raw = _lowered(bgf_arb_system)

    # One fan-out per class stream, one conflict tree per bank.
    routes = [line for line in raw.splitlines() if " = ac.route " in line]
    assert len(routes) == 2 * SOURCE_CLASS_COUNT
    depths = ", ".join(["1"] * BANK_PARTITIONS)
    assert all(f"depths [{depths}] latencies [{depths}]" in line for line in routes)
    assert raw.count(" = ac.merge ") == BANK_PARTITIONS

    # Every selector masks the decoded bank field, so an out-of-range selector
    # is unreachable by construction rather than by upstream promise.
    selectors = re.findall(r'ac\.var\.get %item field "(\w+)"', raw)
    assert selectors == ["bank"] * len(routes)
    assert raw.count(f"ac.var.constant {BANK_INDEX_MASK} : i16") == len(routes)

    # The requested bank is never rewritten into the granted bank.
    assert 'field "bank"' not in re.sub(r'ac\.var\.get %item field "bank"', "", raw)


def test_only_map_writes_the_placement_fields() -> None:
    writers: dict[str, set[str]] = {field: set() for field in PLACEMENT_FIELDS}
    for path in sorted(BGF.glob("*.py")):
        tree = ast.parse(path.read_text(encoding="utf-8"))
        for node in ast.walk(tree):
            if (
                isinstance(node, ast.Call)
                and isinstance(node.func, ast.Attribute)
                and node.func.attr == "with_fields"
            ):
                for keyword in node.keywords:
                    if keyword.arg in writers:
                        writers[keyword.arg].add(path.name)

    assert writers == {field: {"map.py"} for field in PLACEMENT_FIELDS}


def test_lane_collection_keys_are_the_source_class_tags() -> None:
    tree = ast.parse((BGF / "map.py").read_text(encoding="utf-8"))
    collections = [
        node
        for node in ast.walk(tree)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "map"
    ]
    assert len(collections) == 2
    for collection in collections:
        argument = collection.args[0]
        assert isinstance(argument, ast.Dict)
        assert [key.value for key in argument.keys] == list(range(SOURCE_CLASS_COUNT))
