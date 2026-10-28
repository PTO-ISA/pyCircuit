"""Automatic declarations, result wiring and independent current-state reads."""

from __future__ import annotations

import hashlib
import json
import os
import shutil

import pytest
from test_saved_anchor import _save_manifest
from test_source_system_execution import (
    _assert_closed_observation_identities,
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


CHILD_BOUNDARY_PROVIDER = """import pycircuit as ac

@ac.module
def Child(value: ac.u8) -> ac.u8:
    return value

def identity(value: ac.u8) -> ac.u8:
    return value
"""


@pytest.mark.parametrize("capture_mode", ["body", "default"])
@pytest.mark.parametrize(
    ("imports", "child"),
    [
        ("from checks.provider import Child", "Child"),
        ("from checks.provider import Child as ImportedChild", "ImportedChild"),
        ("import checks.provider as provider", "provider.Child"),
    ],
    ids=["direct-import", "alias-import", "namespace-import"],
)
def test_imported_child_is_an_explicit_boundary_between_local_rule_results(
    tmp_path, imports, child, capture_mode
):
    source = tmp_path / "source"
    source.mkdir()
    provider_source = source / "provider.py"
    provider_source.write_text(CHILD_BOUNDARY_PROVIDER)
    parameters = "incoming=forwarded" if capture_mode == "default" else ""
    captured = "incoming" if capture_mode == "default" else "forwarded"
    (source / "consumer.py").write_text(
        f"""import pycircuit as ac
{imports}

@ac.module
def Pipeline(value: ac.u8) -> ac.u8:
    @ac.rule
    def produce() -> ac.u8:
        return value + 1
    forwarded = {child}(produce)
    @ac.rule
    def consume({parameters}) -> ac.u8:
        return {captured} + 1
    return consume

@ac.system
def ExerciseChildBoundary():
    phase: ac.u8 = 0
    incoming = 0 if phase == 0 else 1 if phase == 1 else 17 if phase == 2 else 253
    result = Pipeline(incoming)
    @ac.rule
    def advance() -> None:
        phase = phase + 1
    @ac.rule
    def observe() -> None:
        ac.log("info", "child boundary", incoming, result)
"""
    )
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, "provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-provider.ac"
    provider_source.unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "consumer.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "child-boundary.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.consumer.ExerciseChildBoundary",
            "-o",
            str(final),
        )
    )
    original = final.read_bytes()
    # Child is a combinational identity. Each autonomous callback adds one,
    # so this explicit child interface carries x + 2 without a clock delay.
    expected = [(0, 2), (1, 3), (17, 19), (253, 255)]
    transcripts = []
    for target in ("cpp", "verilog"):
        destination = tmp_path / target
        destination.mkdir()
        binary = _bundle(final, destination, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            logs = [row for row in rows if row["kind"] == "log"]
            assert [row["spec"]["event"] for row in logs] == ["child boundary"] * 8
            assert [
                tuple(int(value["value"]) for value in row["values"]) for row in logs
            ] == [pair for pair in expected for _ in range(2)]
            assert [int(row["evaluation_epoch"]) for row in logs] == list(range(8))
            _assert_closed_observation_identities(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert final.read_bytes() == original


@pytest.mark.parametrize("capture_mode", ["body", "default"])
@pytest.mark.parametrize(
    ("declarations", "expression"),
    [
        ("", "produce"),
        (
            "def identity(value: ac.u8) -> ac.u8:\n    return value\n",
            "identity(produce)",
        ),
        ("from checks.provider import identity\n", "identity(produce)"),
    ],
    ids=["direct-alias", "same-source-helper", "imported-helper"],
)
def test_aliases_and_pure_helpers_do_not_create_child_rule_boundaries(
    tmp_path, declarations, expression, capture_mode
):
    source = tmp_path / "source"
    source.mkdir()
    provider_source = source / "provider.py"
    provider_source.write_text(CHILD_BOUNDARY_PROVIDER)
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-provider.ac"
    provider_source.unlink()
    body.rename(hidden)
    parameters = "incoming=forwarded" if capture_mode == "default" else ""
    captured = "incoming" if capture_mode == "default" else "forwarded"
    try:

        def design(forwarded):
            return f"""import pycircuit as ac
{declarations}
@ac.module
def Pipeline(value: ac.u8) -> ac.u8:
    @ac.rule
    def produce() -> ac.u8:
        return value + 1
    forwarded = {forwarded}
    @ac.rule
    def consume({parameters}) -> ac.u8:
        return {captured} + 1
    return consume
"""

        # The helper/import itself is valid; only feeding it a local Rule result
        # and capturing that derived value in another Rule must be rejected.
        (source / "control.py").write_text(
            design(expression.replace("produce", "value"))
        )
        _ok(_compile(source, "control.py", tmp_path / "control", [provider]))
        (source / "invalid.py").write_text(design(expression))
        output = tmp_path / "rejected"
        result = _compile(source, "invalid.py", output, [provider])
        assert result.returncode != 0, result.stdout + result.stderr
        assert (
            "Traceback" not in result.stderr and "Assertion failed" not in result.stderr
        )
        assert any(
            term in result.stderr.lower() for term in ("local rule", "forwarded")
        ), result.stderr
        assert not (output / "unit.json").exists()
    finally:
        hidden.rename(body)


def _provider(reverse: bool) -> str:
    writer = """
    @ac.rule
    def advance(owned=count, amount=delta) -> None:
        owned = owned + amount
"""
    observer = """
    @ac.rule
    def unused(snapshot=saved) -> ac.u8:
        ac.log("info", "unused result", snapshot)
        return snapshot
"""
    return (
        """import pycircuit as ac

@ac.struct
class Packet:
    value: ac.u8
    check: ac.u8

@ac.rule
def template(value: ac.u8) -> ac.u8:
    return value + 9

@ac.rule
def dormant() -> None:
    assert False, "unbound global templates do not execute"

@ac.module
def Identity(value: ac.u8) -> ac.u8:
    return value

@ac.module
def Dataflow(value: ac.u8) -> tuple[ac.u8, ac.u8, Packet, ac.u8, ac.u8, ac.u8]:
    @ac.rule
    def scalar(argument=value) -> ac.u8:
        return argument + 1
    @ac.rule
    def packet(argument: ac.u8=value) -> Packet:
        return Packet(value=argument + 2, check=argument + 3)
    @ac.rule
    def pair(argument=value) -> tuple[ac.u8, ac.u8]:
        return argument + 4, argument + 5
    left, right = pair
    copied = Identity(scalar)
    bound_template = template(value)
    return scalar, copied, packet, left, right, bound_template

@ac.module
def Counter(delta: ac.u8) -> ac.u8:
    count: ac.u8 = 7
    saved = count
"""
        + (observer + writer if reverse else writer + observer)
        + """
    @ac.method
    def read(delta: ac.u8) -> ac.u8:
        delta = delta + 1
        return count + delta
    return count
"""
    )


BENCH = """import pycircuit as ac
from checks.provider import Counter, Dataflow

@ac.system
def ExerciseAutomaticRules():
    phase: ac.u8 = 0
    first = Counter(1)
    second = Counter(3)
    scalar, copied, packet, left, right, bound_template = Dataflow(phase)
    @ac.rule
    def tick() -> None:
        phase = phase + 1
    @ac.rule
    def observe() -> None:
        method_value = first.read(4)
        ac.log("info", "automatic values", phase, first, second,
               scalar, copied, packet.value, packet.check,
               left, right, bound_template, method_value)
"""


@pytest.mark.parametrize("reverse", [False, True], ids=["writer-first", "reader-first"])
def test_automatic_rules_keep_one_activation_fanout_and_current_reads(
    tmp_path, reverse
):
    source = tmp_path / "source with spaces"
    source.mkdir()
    (source / "provider.py").write_text(_provider(reverse))
    (source / "bench.py").write_text(BENCH)
    provider, bench = tmp_path / "provider", tmp_path / "bench"
    _ok(_compile(source, "provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-provider.ac"
    (source / "provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "bench.py", bench, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "automatic.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.bench.ExerciseAutomaticRules",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # Each cycle has two Work observations. Rising edges commit one update:
    # phase += 1, first += 1, second += 3. All callbacks read pre-edge Q.
    expected = [
        (0, 7, 7, 1, 1, 2, 3, 4, 5, 9, 12),
        (1, 8, 10, 2, 2, 3, 4, 5, 6, 10, 13),
        (2, 9, 13, 3, 3, 4, 5, 6, 7, 11, 14),
    ]
    transcripts = []
    for target in ("cpp", "verilog"):
        destination = tmp_path / target
        destination.mkdir()
        binary = _bundle(final, destination, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "3", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *args], timeout=30)))
            values = [
                row
                for row in rows
                if row["kind"] == "log" and row["spec"]["event"] == "automatic values"
            ]
            assert [
                tuple(int(value["value"]) for value in row["values"]) for row in values
            ] == [value for value in expected for _ in range(2)]
            unused = [
                row
                for row in rows
                if row["kind"] == "log" and row["spec"]["event"] == "unused result"
            ]
            # An unused valued declaration still runs exactly once per instance
            # and epoch. The saved binding is re-evaluated, never frozen at 7.
            by_instance = {}
            for row in unused:
                by_instance.setdefault(row["instance"], []).append(
                    int(row["values"][0]["value"])
                )
            assert sorted(by_instance.values()) == [
                [7, 7, 8, 8, 9, 9],
                [7, 7, 10, 10, 13, 13],
            ]
            _assert_closed_observation_identities(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


@pytest.mark.parametrize(
    "body",
    [
        "    step()\n    return step\n",
        "    alias = step\n    return alias()\n",
        "    step = value\n    return step\n",
        "    @ac.rule\n    def consume() -> ac.u8:\n        return step\n    return consume\n",
        "    alias = step\n    @ac.rule\n    def consume() -> ac.u8:\n        return alias\n    return consume\n",
        "    @ac.rule\n    def consume(argument=step) -> ac.u8:\n        return argument\n    return consume\n",
        "    def helper() -> ac.u8:\n        return step\n    @ac.rule\n    def consume() -> ac.u8:\n        return helper()\n    return consume\n",
    ],
    ids=[
        "explicit-call",
        "alias-call",
        "rebind",
        "direct-dependency",
        "aliased-dependency",
        "default-dependency",
        "helper-dependency",
    ],
)
def test_local_rule_results_cannot_hide_callable_or_callback_dependencies(
    tmp_path, body
):
    source = tmp_path / "source"
    source.mkdir()
    (source / "invalid.py").write_text(
        """import pycircuit as ac
@ac.module
def Invalid(value: ac.u8) -> ac.u8:
    @ac.rule
    def step(argument=value) -> ac.u8:
        return argument + 1
"""
        + body
    )
    result = _compile(source, "invalid.py", tmp_path / "unit")
    assert result.returncode != 0, result.stdout + result.stderr
    diagnostic = result.stderr.lower()
    expected = "call" if "alias()" in body else "rule"
    assert expected in diagnostic, result.stderr
    assert not (tmp_path / "unit" / "unit.json").exists()


def test_retired_after_cannot_return_through_saved_anchor_or_final_metadata(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    (source / "counter.py").write_text(
        """import pycircuit as ac
@ac.system
def Counter():
    count: ac.u8 = 5
    @ac.rule
    def advance() -> None:
        count = count + 1
    @ac.rule
    def observe() -> None:
        ac.log("info", "current", count)
"""
    )
    unit, anchor, final = tmp_path / "unit", tmp_path / "anchor", tmp_path / "final.ac"
    _ok(_compile(source, "counter.py", unit))
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.counter.Counter",
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    original = final.read_bytes()
    shutil.rmtree(source)
    shutil.rmtree(unit)

    for location, field in (
        ("graph", "ac.after"),
        ("graph", "ac.callable_after"),
        ("header", "after"),
        ("header", "callable_after"),
    ):
        bad = tmp_path / f"{location}-{field}"
        shutil.copytree(anchor, bad)
        manifest = json.loads((bad / "anchor.json").read_text())
        if location == "graph":
            relative = "graph.ac"
            needle = "module attributes {"
            replacement = needle + f"{field} = [], "
        else:
            relative = manifest["sources"][0]["interface"]
            needle = "registration_entry ="
            replacement = f"{field} = [], " + needle
        path = bad / relative
        text = path.read_text()
        assert needle in text
        path.write_text(text.replace(needle, replacement, 1))
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        next(row for row in manifest["files"] if row["path"] == relative)[
            "sha256"
        ] = digest
        if location == "graph":
            manifest["configuration"]["graph_sha256"] = digest
        else:
            manifest["sources"][0]["interface_sha256"] = digest
        # Re-sign the ordinary manifest envelope so rejection proves native
        # semantic admission, rather than a stale payload checksum.
        _save_manifest(bad, manifest)
        for result in (
            _cli(
                "link",
                "--from-anchor",
                str(bad),
                "--target",
                "final",
                "-o",
                str(final),
                "--replace",
            ),
            _run(
                [
                    os.environ["PYCIRCUIT_LINKER"],
                    "--from-anchor",
                    str(bad),
                    "--verify-only",
                ]
            ),
        ):
            assert result.returncode != 0, result.stdout + result.stderr
            # callable_after is an extra autonomous-row key, so the closed
            # schema rejects it before the retired-ordering semantic check.
            category = (
                "transaction summary has missing or extra fields"
                if location == "header" and field == "callable_after"
                else "after"
            )
            assert category in result.stderr.lower(), result.stderr
        assert final.read_bytes() == original

    text = original.decode()
    needle = "ac.sampled_admission = {"
    assert needle in text
    for field in ("ac.after", "ac.callable_after", "ehr_entries"):
        replacement = (
            needle + "ehr_entries = [], "
            if field == "ehr_entries"
            else f"{field} = [], " + needle
        )
        bad = tmp_path / f"final-{field}.ac"
        bad.write_text(text.replace(needle, replacement, 1))
        # The final package retains its authentic anchor provenance. It must
        # still revalidate retired scheduling metadata on direct native load.
        result = _run(
            [os.environ["PYCIRCUIT_LINKER"], "--design", str(bad), "--verify-only"]
        )
        assert result.returncode != 0, result.stdout + result.stderr
        assert any(
            term in result.stderr.lower() for term in ("after", "ehr")
        ), result.stderr
        assert final.read_bytes() == original
