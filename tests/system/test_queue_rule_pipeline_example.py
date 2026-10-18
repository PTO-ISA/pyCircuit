"""The issue-319 Queue/Rule example completes through both generated backends."""

from __future__ import annotations

import hashlib
import json
import shutil

import pytest
from test_source_system_execution import (
    ROOT,
    _assert_closed_observation_identities,
    _bundle,
    _cli,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system

EXAMPLE = ROOT / "examples/queue_rule_pipeline"
PACKAGE = "example_queue_rule_pipeline"
TOP = f"{PACKAGE}.bench.QueueRulePipelineSystem"


def _compile_source(source, filename, output, providers=()):
    arguments = [
        "compile",
        "-c",
        str(source / filename),
        "--source-root",
        str(source),
        "--package-prefix",
        PACKAGE,
        "-o",
        str(output),
    ]
    for provider in providers:
        arguments += ["-I", str(provider)]
    return _cli(*arguments)


def _program(tmp_path):
    source = tmp_path / "queue rule pipeline sources"
    source.mkdir()
    for filename in ("queue_rule_pipeline.py", "bench.py"):
        shutil.copy2(EXAMPLE / filename, source)

    provider = tmp_path / "queue rule provider"
    consumer = tmp_path / "queue rule consumer"
    _ok(_compile_source(source, "queue_rule_pipeline.py", provider))

    receipt = json.loads((provider / "unit.json").read_text())
    interface = provider / receipt["files"]["interface"]
    body = provider / receipt["files"]["body"]
    provider_source = source / "queue_rule_pipeline.py"
    hidden_body = tmp_path / "unavailable queue rule provider body.ac"
    provider_source.unlink()
    body.rename(hidden_body)
    try:
        _ok(_compile_source(source, "bench.py", consumer, [provider]))
    finally:
        hidden_body.rename(body)

    consumer_receipt = json.loads((consumer / "unit.json").read_text())
    dependencies = (consumer / consumer_receipt["files"]["depfile"]).read_text()

    def depfile_path(path):
        return str(path).replace(" ", "\\ ")

    assert depfile_path(interface) in dependencies
    assert depfile_path(body) not in dependencies
    assert depfile_path(provider_source) not in dependencies

    final = tmp_path / "queue-rule-pipeline.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            TOP,
            "-o",
            str(final),
        )
    )
    return final


def _values(rows, event):
    return [
        (
            int(row["evaluation_epoch"]),
            tuple(int(value["value"]) for value in row["values"]),
        )
        for row in rows
        if row["kind"] == "log" and row["spec"]["event"] == event
    ]


def _assert_generated_source(generated):
    receipt = json.loads((generated / "generated.json").read_text())
    sources = [
        generated / item["path"]
        for item in receipt["files"]
        if (generated / item["path"]).suffix in {".cpp", ".hpp", ".sv", ".v"}
    ]
    assert sources
    text = "\n".join(path.read_text() for path in sources)
    for event in ("queue progress", "queue received", "queue committed"):
        assert event in text
    if receipt["target"] == "cpp":
        for message in (
            "queue pipeline: packet order",
            "queue pipeline: accumulated payload",
            "queue pipeline: producer held by backpressure",
            "queue pipeline: all outputs consumed",
        ):
            assert message in text
        assert "source_check_failed" in text
    else:
        # Verilog reduces authored checks to the shared generated error tree;
        # messages remain in final IR while every check still drives hardware.
        assert receipt["target"] == "verilog"
        assert text.count("pyc_check_error_") >= 10
        assert "pyc_local_error" in text
    return receipt


def test_queue_rule_pipeline_example_runs_one_verified_artifact(tmp_path):
    final = _program(tmp_path)
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    final_text = final.read_text()
    assert final_text.count('kind = "assert"') >= 10
    assert "queue progress" in final_text

    # Independent old-Q schedule. Each physical cycle publishes one low and one
    # rising-edge Work sample before the rising-edge state becomes visible.
    committed = [
        (0, 0, 0, 0, 0),
        (1, 1, 0, 0, 0),
        (2, 2, 0, 12, 1),
        (3, 2, 0, 12, 1),
        (4, 2, 0, 12, 1),
        (5, 2, 0, 12, 1),
        (6, 2, 0, 12, 1),
        (7, 3, 1, 25, 2),
        (8, 4, 2, 39, 3),
        (9, 4, 3, 54, 4),
        (10, 4, 4, 54, 4),
        (11, 4, 4, 54, 4),
    ]
    progress = [0, 1, 1, 1, 1, 1, 2, 3, 4, 4, 4, 4]
    received = [
        (6, 0, 12),
        (7, 1, 25),
        (8, 2, 39),
        (9, 3, 54),
    ]
    expected_committed = [
        (2 * cycle + edge, values)
        for cycle, values in enumerate(committed)
        for edge in (0, 1)
    ]
    expected_progress = [
        (2 * cycle + edge, (value,))
        for cycle, value in enumerate(progress)
        for edge in (0, 1)
    ]
    expected_received = [
        (2 * cycle + edge, values)
        for cycle, values in zip(range(6, 10), received, strict=True)
        for edge in (0, 1)
    ]

    transcripts = []
    manifests = {}
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        manifests[target] = _assert_generated_source(target_root / target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "12", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=12"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            assert _values(rows, "queue committed") == expected_committed
            assert _values(rows, "queue progress") == expected_progress
            assert _values(rows, "queue received") == expected_received
            assert all(
                int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
                for row in rows
                if row["kind"] != "result"
            )
            result = [row for row in rows if row["kind"] == "result"]
            assert len(result) == 1
            assert result[0]["status"] == "TERMINATED"
            assert int(result[0]["epoch_time"]) == 24
            assert result[0]["error"] is None
            _assert_closed_observation_identities(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])

    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert manifests["cpp"]["entry"] == manifests["verilog"]["entry"]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
