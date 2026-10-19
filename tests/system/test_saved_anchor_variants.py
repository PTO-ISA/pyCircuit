"""Saved graph variants preserve provider identity and compact storage families."""

from __future__ import annotations

import hashlib
import json
import re
import shutil

import pytest
from test_saved_anchor import _tree
from test_source_system_execution import (
    FIXTURE,
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def _refine_and_observe(anchor, root, event, expected):
    before = _tree(anchor)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = root / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        build = root / target
        build.mkdir()
        binary = _bundle(final, build, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", str(len(expected)), "--workers", str(workers)]
                if target == "cpp"
                else [f"+cycles={len(expected)}"]
            )
            records = _records(_ok(_run([str(binary), *args], timeout=30)))
            logs = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == event
            ]
            assert [
                tuple(int(value["value"]) for value in row["values"]) for row in logs
            ] == [value for value in expected for _ in range(2)]
            assert [int(row["evaluation_epoch"]) for row in logs] == list(
                range(2 * len(expected))
            )
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert _tree(anchor) == before
    return final


def test_same_provider_keeps_immutable_source_and_distinct_caller_variants(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    for filename in (
        "anchor_variant_provider.py",
        "anchor_variant_read.py",
        "anchor_variant_write.py",
    ):
        shutil.copy2(FIXTURE / filename, source)
    provider = tmp_path / "provider"
    _ok(_compile(source, "anchor_variant_provider.py", provider))
    provider_bytes = _tree(provider)
    provider_receipt = json.loads((provider / "unit.json").read_text())
    provider_body = provider / provider_receipt["files"]["body"]
    # Both consumers compile against the same immutable provider interface.
    (source / "anchor_variant_provider.py").unlink()
    hidden_body = tmp_path / "hidden-body.ac"
    provider_body.rename(hidden_body)
    consumers = []
    try:
        for kind in ("read", "write"):
            consumer = tmp_path / (kind + "-unit")
            _ok(_compile(source, f"anchor_variant_{kind}.py", consumer, [provider]))
            consumers.append(consumer)
    finally:
        hidden_body.rename(provider_body)
    anchors = []
    definitions = []
    signatures = []
    for kind, top, consumer in zip(
        ("read", "write"), ("ReadCounter", "WriteCounter"), consumers, strict=True
    ):
        anchor = tmp_path / (kind + "-anchor")
        _ok(
            _cli(
                "link",
                str(provider),
                str(consumer),
                "--top",
                f"checks.anchor_variant_{kind}.{top}",
                "--target",
                "anchor",
                "-o",
                str(anchor),
            )
        )
        assert _tree(provider) == provider_bytes
        manifest = json.loads((anchor / "anchor.json").read_text())
        owned = [
            row
            for row in manifest["definitions"]
            if row["source"]
            == {"package": "checks", "path": "anchor_variant_provider.py"}
        ]
        assert len(owned) == 1
        definition = owned[0]
        spec = (anchor / definition["spec"]).read_text()
        body = (anchor / definition["body"]).read_bytes()
        assert hashlib.sha256(spec.encode()).hexdigest() == definition["spec_sha256"]
        assert hashlib.sha256(body).hexdigest() == definition["body_sha256"]
        signatures.append(
            tuple(
                re.search(rf"{name} = (\[[^\]]*\])", spec).group(1)
                for name in ("input_names", "output_names")
            )
        )
        definitions.append(definition)
        anchors.append(anchor)
    assert definitions[0]["definition"] == definitions[1]["definition"]
    assert (
        signatures[0] != signatures[1]
    ), "actual caller demands must change routed ports"
    for identity in ("variant_id", "spec_sha256", "body_sha256"):
        assert definitions[0][identity] != definitions[1][identity]
    assert _tree(provider) == provider_bytes
    for directory in (source, provider, *consumers):
        shutil.rmtree(directory)
    for kind, anchor, expected in zip(
        ("read", "write"),
        anchors,
        ([(7,), (7,), (7,)], [(7, 10, 7), (10, 13, 7), (13, 16, 7)]),
        strict=True,
    ):
        root = tmp_path / (kind + "-reload")
        root.mkdir()
        portable = root / "portable"
        shutil.copytree(anchor, portable)
        shutil.rmtree(anchor)
        _refine_and_observe(portable, root, "anchor " + kind, expected)


def test_saved_anchor_keeps_table_state_compact_through_refinement(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    filename = "anchor_collection_bench.py"
    shutil.copy2(FIXTURE / filename, source)
    unit = tmp_path / "unit"
    _ok(_compile(source, filename, unit))
    anchor = tmp_path / "anchor"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.anchor_collection_bench.ExerciseAnchorCollection",
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert len(manifest["definitions"]) == 1
    body = (anchor / manifest["definitions"][0]["body"]).read_text()
    # Routing represents a persistent Table as one logical state. This checks
    # compact state refinement, not a retained native ac.collection in the anchor.
    states = [line for line in body.splitlines() if '"ac.state"(' in line]
    assert len(states) == 2  # One four-lane state plus the scalar position.
    table_states = [line for line in states if 'instance_name = "rows"' in line]
    assert len(table_states) == 1
    assert re.search(r"!ac.table<\[.*#ac.math_int<4>", table_states[0])
    assert not re.search(r'instance_name = "rows[._\[]', body)
    shutil.rmtree(source)
    shutil.rmtree(unit)
    root = tmp_path / "reload"
    root.mkdir()
    portable = root / "portable"
    shutil.copytree(anchor, portable)
    shutil.rmtree(anchor)
    expected = [
        (0, 2, 5, 2, 2, 2),
        (1, 2, 5, 5, 2, 2),
        (2, 2, 5, 5, 5, 2),
        (3, 2, 5, 5, 5, 5),
        (0, 5, 8, 5, 5, 5),
        (1, 5, 8, 8, 5, 5),
        (2, 5, 8, 8, 8, 5),
        (3, 5, 8, 8, 8, 8),
    ]
    final = _refine_and_observe(portable, root, "anchor collection", expected)
    final_collections = [
        line for line in final.read_text().splitlines() if '"ac.collection"(' in line
    ]
    assert len(final_collections) == 1
    assert 'instance_name = "rows"' in final_collections[0]
    assert re.search(
        r"shape = \[.*#ac.math_int<4>.*\], type_arguments", final_collections[0]
    )
