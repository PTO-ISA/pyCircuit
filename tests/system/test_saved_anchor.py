"""Public saved anchors survive source removal and retain independent execution."""

from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import sys

import pytest
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
TOP = "checks.saved_anchor_bench.ExerciseSavedAnchor"
LEGACY_TOP = "checks.method_nominal_bench.ExerciseNominalMethods"


def _tree(directory):
    return {
        str(path.relative_to(directory)): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file()
    }


def _json(value):
    return json.dumps(
        value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ).encode()


def _save_manifest(directory, manifest):
    manifest["configuration_identity"] = hashlib.sha256(
        _json(manifest["configuration"])
    ).hexdigest()
    manifest["content_identity"] = hashlib.sha256(
        _json(
            {key: value for key, value in manifest.items() if key != "content_identity"}
        )
    ).hexdigest()
    (directory / "anchor.json").write_bytes(_json(manifest) + b"\n")


def _publish(tmp_path, *, remove_sources=True, legacy=False):
    source = tmp_path / "source"
    source.mkdir()
    prefix = "method_nominal" if legacy else "saved_anchor"
    provider_file, consumer_file = f"{prefix}_provider.py", f"{prefix}_bench.py"
    for filename in (provider_file, consumer_file):
        shutil.copy2(FIXTURE / filename, source)
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, provider_file, provider))
    # Consumer capture receives only the admitted interface, never provider code.
    (source / provider_file).unlink()
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-body.ac"
    body.rename(hidden)
    try:
        _ok(_compile(source, consumer_file, consumer, [provider]))
    finally:
        hidden.rename(body)
    unit_bytes = (_tree(provider), _tree(consumer))
    anchor, duplicate = tmp_path / "anchor", tmp_path / "duplicate"
    for output, units in (
        (anchor, (provider, consumer)),
        (duplicate, (consumer, provider)),
    ):
        _ok(
            _cli(
                "link",
                *(str(unit) for unit in units),
                "--top",
                LEGACY_TOP if legacy else TOP,
                "--target",
                "anchor",
                "-o",
                str(output),
            )
        )
    assert _tree(anchor) == _tree(
        duplicate
    ), "source argument order must not affect routed variants"
    assert unit_bytes == (_tree(provider), _tree(consumer))
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert '"ac.module"' not in (anchor / "graph.ac").read_text()
    assert 'ac.stage = "routed"' in (anchor / "graph.ac").read_text()
    assert len({row["definition"] for row in manifest["definitions"]}) == len(
        manifest["definitions"]
    )
    assert any(
        '"ac.state.update"' in (anchor / row["body"]).read_text()
        for row in manifest["definitions"]
    )
    if remove_sources:
        for directory in (source, provider, consumer):
            shutil.rmtree(directory)
    return anchor


@pytest.fixture
def saved_anchor(tmp_path):
    return _publish(tmp_path)


@pytest.fixture
def legacy_saved_anchor(tmp_path):
    # Retained nominal/candidate-forwarding migration oracle; not replaced by
    # the bounded transactional counter scenario.
    return _publish(tmp_path, legacy=True)


@pytest.mark.skipif(os.name == "nt", reason='Windows filenames cannot contain "')
def test_saved_anchor_roundtrip_with_escaped_source_owner_paths(tmp_path):
    _publish(tmp_path, remove_sources=False)
    source = tmp_path / "source"
    units = [tmp_path / "provider", tmp_path / "consumer"]
    for index, filename in enumerate(('a".py', "a-.py")):
        # These are real independently compiled source units, not fabricated
        # provenance rows. Empty headers retain an admitted interface owner.
        (source / filename).write_text("# Empty source interface owner.\n")
        unit = tmp_path / f"empty-{index}"
        _ok(_compile(source, filename, unit))
        units.append(unit)
    directory = tmp_path / "escaped-anchor"
    _ok(
        _cli(
            "link",
            *(str(unit) for unit in units),
            "--top",
            TOP,
            "--target",
            "anchor",
            "-o",
            str(directory),
        )
    )
    manifest = json.loads((directory / "anchor.json").read_text())
    # Transactional anchors retain source owners in the authenticated Linked
    # snapshot rather than a separate source-interface inventory.
    assert manifest["schema"] == 3
    assert manifest["sources"] == []
    graph = (directory / "graph.ac").read_text()
    snapshot = re.search(
        r'ac.transactional_source = "((?:\\[0-9A-Fa-f]{2}|\\\\|[^"\\])*)"', graph
    )
    assert snapshot is not None
    linked = re.sub(
        r"\\([0-9A-Fa-f]{2}|\\)",
        lambda match: "\\" if match[1] == "\\" else chr(int(match[1], 16)),
        snapshot[1],
    )
    owners = re.search(r"ac.source_units = \[(.*?)\]", linked)
    assert owners is not None
    paths = [
        re.sub(r"\\([0-9A-Fa-f]{2})", lambda match: chr(int(match[1], 16)), path)
        for path in re.findall(r'path = "((?:\\[0-9A-Fa-f]{2}|[^"\\])*)"', owners[1])
    ]
    assert sorted(paths) == sorted(
        ["a-.py", 'a".py', "saved_anchor_bench.py", "saved_anchor_provider.py"]
    )
    before = _tree(directory)
    for path in (source, *units):
        shutil.rmtree(path)
    _ok(
        _run(
            [
                os.environ["PYCIRCUIT_LINKER"],
                "--from-anchor",
                str(directory),
                "--verify-only",
            ]
        )
    )
    _ok(_cli("link", "--from-anchor", str(directory), "--verify-only"))
    final = tmp_path / "escaped-final.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(directory),
            "--target",
            "final",
            "-o",
            str(final),
        )
    )
    assert 'ac.stage = "final"' in final.read_text()
    assert "ac.anchor_provenance" in final.read_text()
    assert _tree(directory) == before


@pytest.mark.parametrize("target,workers", [("cpp", 1), ("cpp", 2), ("verilog", 1)])
def test_transactional_anchor_continues_after_source_removal(
    tmp_path, saved_anchor, target, workers
):
    relocated = tmp_path / "relocated" / "portable"
    shutil.copytree(saved_anchor, relocated)
    shutil.rmtree(saved_anchor)
    original = _tree(relocated)
    parent_before = sorted(relocated.parent.iterdir())
    _ok(_cli("link", "--from-anchor", str(relocated), "--verify-only"))
    assert sorted(relocated.parent.iterdir()) == parent_before
    final = tmp_path / "from-anchor.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(relocated),
            "--target",
            "final",
            "-o",
            str(final),
        )
    )
    assert _tree(relocated) == original
    assert 'ac.stage = "final"' in final.read_text()
    assert "ac.anchor_provenance" in final.read_text()
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # Old-Q is read even after advance; one rising edge adds three. The
    # denied transaction never changes held. These values follow the source
    # recurrence, independently of either backend or the saved representation.
    expected = {
        "anchor advance": [(5, 8), (8, 11), (11, 14)],
        "anchor old": [(5, 5), (8, 5), (11, 5)],
    }
    root = tmp_path / target
    root.mkdir()
    binary = _bundle(final, root, target)
    arguments = (
        ["--cycles", "3", "--workers", str(workers)]
        if target == "cpp"
        else ["+cycles=3"]
    )
    rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
    for event, values in expected.items():
        selected = [
            row
            for row in rows
            if row["kind"] == "log" and row["spec"]["event"] == event
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in selected
        ] == [value for value in values for _ in range(2)]
        assert [
            (int(row["evaluation_epoch"]), int(row["commit_epoch"])) for row in selected
        ] == [(epoch, epoch + 1) for epoch in range(6)]
    transcript = [row for row in rows if row["kind"] != "result"]
    assert not any(
        row.get("spec", {}).get("event") == "anchor suppressed" for row in rows
    )
    arguments = (
        ["--cycles", "4", "--workers", str(workers)]
        if target == "cpp"
        else ["+cycles=4"]
    )
    failure = _run([str(binary), *arguments], timeout=30)
    assert failure.returncode != 0
    failed_rows = _records(failure.stdout)
    assert [row for row in failed_rows if row["kind"] != "result"] == transcript
    terminal = [row for row in failed_rows if row["kind"] == "result"]
    if terminal:
        assert len(terminal) == 1 and terminal[0]["status"] == "FAILED"
        assert int(terminal[0]["epoch_time"]) == 6
    else:
        assert "check failed at epoch 6" in failure.stdout + failure.stderr
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert _tree(relocated) == original


def test_portable_anchor_refines_and_executes_with_original_rollback_oracle(
    tmp_path, legacy_saved_anchor
):
    relocated = tmp_path / "relocated" / "portable"
    shutil.copytree(legacy_saved_anchor, relocated)
    shutil.rmtree(legacy_saved_anchor)
    original = _tree(relocated)
    parent_before = sorted(relocated.parent.iterdir())
    _ok(_cli("link", "--from-anchor", str(relocated), "--verify-only"))
    assert sorted(relocated.parent.iterdir()) == parent_before
    final = tmp_path / "from-anchor.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(relocated),
            "--target",
            "final",
            "-o",
            str(final),
        )
    )
    assert _tree(relocated) == original
    assert 'ac.stage = "final"' in final.read_text()
    assert "ac.anchor_provenance" in final.read_text()
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    expected = {
        "nominal first": [(7, 16, 6, 0), (10, 19, 9, 0), (13, 22, 12, 0)],
        "nominal second": [(10, 6, 8, 5), (13, 9, 11, 5), (16, 12, 14, 5)],
        "nominal old": [(5, 5), (8, 5), (11, 5)],
    }
    for target in ("cpp", "verilog"):
        root = tmp_path / target
        root.mkdir()
        binary = _bundle(final, root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "3", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            for event, values in expected.items():
                selected = [
                    row
                    for row in rows
                    if row["kind"] == "log" and row["spec"]["event"] == event
                ]
                assert [
                    tuple(int(value["value"]) for value in row["values"])
                    for row in selected
                ] == [value for value in values for _ in range(2)]
                assert [
                    (int(row["evaluation_epoch"]), int(row["commit_epoch"]))
                    for row in selected
                ] == [(epoch, epoch + 1) for epoch in range(6)]
            transcript = [row for row in rows if row["kind"] != "result"]
            assert not any(
                row.get("spec", {}).get("event") == "nominal suppressed" for row in rows
            )
            transcripts.append(transcript)
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            failure = _run([str(binary), *arguments], timeout=30)
            assert failure.returncode != 0
            failed_rows = _records(failure.stdout)
            assert [row for row in failed_rows if row["kind"] != "result"] == transcript
            terminal = [row for row in failed_rows if row["kind"] == "result"]
            if terminal:
                assert len(terminal) == 1 and terminal[0]["status"] == "FAILED"
                assert int(terminal[0]["epoch_time"]) == 6
                gauge = [
                    row
                    for row in terminal[0]["statistics"]
                    if row["name"] == "nominal count"
                ]
                assert len(gauge) == 1 and int(gauge[0]["value"]) == 14
                assert int(gauge[0]["last_update"]["time"]) == 6
            else:
                assert "check failed at epoch 6" in failure.stdout + failure.stderr
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert _tree(relocated) == original


@pytest.mark.parametrize(
    "mutation", ["domain", "reset", "root", "dependency", "missing-body", "extra-file"]
)
def test_mutated_anchor_rejects_in_public_and_native_loaders_without_replacement(
    tmp_path, saved_anchor, mutation
):
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(saved_anchor),
            "--target",
            "final",
            "-o",
            str(final),
        )
    )
    original = final.read_bytes()
    bad = tmp_path / "bad"
    shutil.copytree(saved_anchor, bad)
    manifest = json.loads((bad / "anchor.json").read_text())
    if mutation == "domain":
        manifest["configuration"]["domain"] = "substituted-domain"
    elif mutation == "reset":
        policy = manifest["configuration"]["reset_semantics"]
        manifest["configuration"]["reset_semantics"] = (
            "if_one" if policy == "strict" else "strict"
        )
    elif mutation == "root":
        manifest["configuration"]["root"]["definition"] = '@"checks.Missing"'
    elif mutation == "dependency":
        manifest["definitions"][0]["dependencies"] = ['@"checks.Missing"']
    elif mutation == "missing-body":
        (bad / manifest["definitions"][0]["body"]).unlink()
    else:
        (bad / "unlisted.ac").write_text("unlisted")
    _save_manifest(bad, manifest)
    result = _cli(
        "link",
        "--from-anchor",
        str(bad),
        "--target",
        "final",
        "-o",
        str(final),
        "--replace",
    )
    assert result.returncode != 0 and "Traceback" not in result.stderr
    assert final.read_bytes() == original
    result = _run(
        [os.environ["PYCIRCUIT_LINKER"], "--from-anchor", str(bad), "--verify-only"]
    )
    assert result.returncode != 0


@pytest.mark.parametrize(
    "extra",
    [
        ("--top", TOP),
        ("--target", "final"),
        ("--reset-semantics", "strict"),
        ("--parameters", "absent.json"),
    ],
)
def test_anchor_verification_rejects_rebinding_options(tmp_path, saved_anchor, extra):
    before = _tree(saved_anchor)
    result = _cli("link", "--from-anchor", str(saved_anchor), "--verify-only", *extra)
    assert result.returncode != 0 and "Traceback" not in result.stderr
    assert _tree(saved_anchor) == before


@pytest.mark.skipif(
    os.name == "nt", reason="Existing SIGKILL recovery harness is POSIX-only"
)
def test_anchor_publication_recovers_each_replacement_crash(tmp_path):
    from test_publication_process_recovery import (
        PUBLICATION_POINTS,
        _assert_control_clean,
        _assert_replacement_crash_shape,
        _crash_at,
        _lock_identity,
    )

    destination = _publish(tmp_path, remove_sources=False)
    before = _tree(destination)
    arguments = [
        "link",
        str(tmp_path / "provider"),
        str(tmp_path / "consumer"),
        "--top",
        TOP,
        "--target",
        "anchor",
        "--reset-semantics",
    ]
    expected = tmp_path / "new-policy"
    _ok(_cli(*arguments, "if_one", "-o", str(expected)))
    after = _tree(expected)
    assert before != after
    for point in PUBLICATION_POINTS:
        _ok(_cli(*arguments, "strict", "-o", str(destination), "--replace"))
        assert _tree(destination) == before
        lock_identity = _lock_identity(destination)
        command = [
            sys.executable,
            "-m",
            "pycircuit.cli",
            *arguments,
            "if_one",
            "-o",
            str(destination),
            "--replace",
        ]
        _crash_at(command, point, env=os.environ.copy(), cwd=tmp_path)
        _assert_replacement_crash_shape(
            destination,
            artifact="anchor",
            target=None,
            point=point,
            old_bytes=before,
            new_bytes=after,
            lock_identity=lock_identity,
        )
        _ok(_cli("link", "--from-anchor", str(destination), "--verify-only"))
        assert _tree(destination) == (
            after if point == "after_journal_committed" else before
        )
        if point == "after_journal_committed":
            # Existing shared reads accept committed bytes without taking an
            # exclusive cleanup lock. The next writer finishes that cleanup.
            _ok(_cli(*arguments, "if_one", "-o", str(destination), "--replace"))
            assert _tree(destination) == after
        _assert_control_clean(destination, lock_identity)
