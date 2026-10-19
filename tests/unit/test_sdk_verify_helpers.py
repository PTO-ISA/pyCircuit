"""Current SDK identity and verifier path helpers."""

from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
VERIFIER = ROOT / "packaging/sdk/verify_platform_candidate.py"
CONTRACT_CHECKER = ROOT / "packaging/sdk/check_contract.py"
CAPABILITIES = [
    "pycircuit-pythonic-source",
    "pycircuit-source-units",
    "pycircuit-cpp",
    "pycircuit-verilog",
    "pyc6-runtime-v1",
]


@pytest.fixture(scope="module")
def verifier():
    spec = importlib.util.spec_from_file_location("pycircuit_sdk_verifier", VERIFIER)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load the SDK verifier")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture(scope="module")
def contract_checker():
    spec = importlib.util.spec_from_file_location(
        "pycircuit_sdk_contract_checker", CONTRACT_CHECKER
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load the SDK contract checker")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_sdk_identity_pins_the_current_distribution_and_contract_versions() -> None:
    version_map = json.loads(
        (ROOT / "packaging/sdk/version-map.json").read_text(encoding="utf-8")
    )
    schema = json.loads(
        (ROOT / "schemas/sdk-manifest.schema.json").read_text(encoding="utf-8")
    )

    assert version_map["schema"] == "pycircuit-sdk-version-map"
    assert version_map["compatibility"] == "exact_identity_tuple"
    assert set(version_map["distributions"]) == {"pycircuit-hisi"}
    assert version_map["contracts"] == {
        "sdk_manifest": "1",
        "generator_abi": "2",
        "runtime_abi": "1",
        "consumer_lock": "1",
        "release_index": "1",
    }
    assert schema["properties"]["capabilities"]["const"] == CAPABILITIES
    assert schema["properties"]["distributions"]["$ref"] == "#/$defs/distributions"
    assert schema["$defs"]["distributions"]["required"] == ["pycircuit-hisi"]


@pytest.mark.parametrize("location", ["schema", "document"])
@pytest.mark.parametrize("token", ["fingerprint", "sha256", "checksum", "digest"])
def test_sdk_contract_rejects_content_identity_tokens(
    contract_checker, location: str, token: str
) -> None:
    schema = {"properties": {"source_revision": {"type": "string"}}}
    document = {"source_revision": "a" * 40}
    target = schema if location == "schema" else document
    target[token] = "not a stable content identity"

    with pytest.raises(ValueError, match=f"forbidden content-identity token: {token}"):
        contract_checker.reject_content_identity(schema, document)


def test_sdk_contract_allows_a_hex_source_revision(
    contract_checker,
) -> None:
    schema = {"properties": {"source_revision": {"type": "string"}}}
    document = {"source_revision": "0123456789abcdef" * 2 + "01234567"}

    contract_checker.reject_content_identity(schema, document)


def test_canonical_path_resolves_an_existing_directory(
    verifier, tmp_path: Path
) -> None:
    target = tmp_path / "nested" / "dir"
    target.mkdir(parents=True)

    resolved = verifier.canonical_path(target)

    assert resolved.is_absolute()
    assert resolved.is_dir()
    assert resolved == target.resolve()


def test_installed_console_script_finds_the_single_py_circuit_driver(
    verifier, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    driver = tmp_path / "pycircuit"
    driver.write_text("", encoding="utf-8")

    monkeypatch.setattr(verifier.sys, "platform", "darwin")
    assert verifier.installed_console_script(tmp_path, "pycircuit") == driver
    assert verifier.installed_console_script(tmp_path, "acc.py") is None

    windows_driver = tmp_path / "pycircuit.exe"
    windows_driver.write_text("", encoding="utf-8")
    monkeypatch.setattr(verifier.sys, "platform", "win32")
    assert verifier.installed_console_script(tmp_path, "pycircuit") == windows_driver
    assert verifier.installed_console_script(tmp_path, "missing") is None


def test_run_names_a_command_that_cannot_start(verifier, tmp_path: Path) -> None:
    with pytest.raises(ValueError, match="command could not start"):
        verifier.run(["definitely-not-a-real-binary-xyz"], cwd=tmp_path)


def _statistic(name: str, kind: str, value: int, owner: str) -> dict[str, Any]:
    # runtime/sim_executor.cpp: appendStatistic and buildStatistics; observation
    # values use strings, whereas statistics values and times are JSON numbers.
    return {
        "buckets": [],
        "count": 0,
        "kind": kind,
        "last_update": {"delta": 0, "time": 10},
        "maximum": 0,
        "minimum": 0,
        "name": name,
        "object_path": owner,
        "sum": 0,
        "value": value,
    }


@pytest.fixture
def system_records() -> list[dict[str, Any]]:
    """Independent old-Q oracle for five low/high saturating-counter cycles.

    Schema comes from runtime/system_runner.cpp eventRecord/resultRecord and
    the statistics writer, not the SDK verifier's expected-record construction.
    This is test-owned transcript data, not generated-model execution evidence.
    """
    records = []
    count = 0
    previous_clock = 0
    for _cycle in range(5):
        for clock in (0, 1):
            epoch = len(records)
            records.append(
                {
                    "kind": "report",
                    "instance": "root",
                    "registration": '@"sdk.design_top.DesignTop"[0]',
                    "site": '@"sdk.design_top.DesignTop"[1]',
                    "evaluation_epoch": str(epoch),
                    "commit_epoch": str(epoch + 1),
                    "spec": {"name": "count"},
                    "values": [{"kind": "integer", "value": str(count)}],
                }
            )
            if previous_clock == 0 and clock == 1:
                count = min(count + 1, 3)
            previous_clock = clock
    records.append(
        {
            "kind": "result",
            "status": "TERMINATED",
            "epoch_time": "10",
            "statistics": [
                _statistic("cycles", "counter", 10, "@runtime"),
                _statistic("stop_reason", "gauge", 1, "@runtime"),
                _statistic("count", "gauge", count, "root"),
            ],
            "error": None,
            "reset_semantics": "strict",
        }
    )
    return records


def _write_records(path: Path, records: list[dict[str, Any]]) -> Path:
    path.write_text(
        "".join(json.dumps(record, separators=(",", ":")) + "\n" for record in records),
        encoding="utf-8",
    )
    return path


def test_system_events_accepts_five_cycles_and_saturated_hold(
    verifier, tmp_path: Path, system_records: list[dict[str, Any]]
) -> None:
    verifier.verify_system_events(
        _write_records(tmp_path / "events.jsonl", system_records)
    )


@pytest.mark.parametrize(
    ("field", "replacement"),
    [
        ("kind", "log"),
        ("instance", "root.counter"),
        ("spec", {"name": "unrelated"}),
        ("values", []),
        ("values", [{"kind": "integer", "value": "2"}]),
        ("values", [{"kind": "bool", "value": False}]),
        ("values", [{"kind": "integer", "value": 0}]),
        (
            "values",
            [{"kind": "bits4", "width": 8, "value": "00", "known": "00", "z": "00"}],
        ),
        (
            "values",
            [{"kind": "bits4", "width": 8, "value": "00", "known": "00", "z": "ff"}],
        ),
        (
            "values",
            [{"kind": "integer", "value": "0"}, {"kind": "integer", "value": "0"}],
        ),
        ("evaluation_epoch", "1"),
        ("commit_epoch", "2"),
    ],
)
def test_system_events_rejects_wrong_report_identity_payload_or_epoch(
    verifier,
    tmp_path: Path,
    system_records: list[dict[str, Any]],
    field: str,
    replacement: object,
) -> None:
    system_records[0][field] = replacement
    with pytest.raises(ValueError):
        verifier.verify_system_events(
            _write_records(tmp_path / "events.jsonl", system_records)
        )


@pytest.mark.parametrize("epoch", [6, 7, 8, 9])
def test_system_events_checks_every_saturated_hold_report(
    verifier, tmp_path: Path, system_records: list[dict[str, Any]], epoch: int
) -> None:
    system_records[epoch]["values"][0]["value"] = "4"
    with pytest.raises(ValueError):
        verifier.verify_system_events(
            _write_records(tmp_path / "events.jsonl", system_records)
        )


@pytest.mark.parametrize(
    "mutation",
    [
        "empty",
        "missing-report",
        "missing-result",
        "extra-report",
        "extra-result",
        "non-object",
        "truncated-json",
    ],
)
def test_system_events_rejects_missing_extra_and_truncated_data(
    verifier,
    tmp_path: Path,
    system_records: list[dict[str, Any]],
    mutation: str,
) -> None:
    if mutation == "empty":
        system_records.clear()
    elif mutation == "missing-report":
        del system_records[4]
    elif mutation == "missing-result":
        system_records.pop()
    elif mutation == "extra-report":
        system_records.insert(0, system_records[0])
    elif mutation == "extra-result":
        system_records.append(system_records[-1])
    elif mutation == "non-object":
        system_records[0] = []
    path = _write_records(tmp_path / "events.jsonl", system_records)
    if mutation == "truncated-json":
        path.write_text(path.read_text(encoding="utf-8")[:-5], encoding="utf-8")
    with pytest.raises(ValueError):
        verifier.verify_system_events(path)


@pytest.mark.parametrize(
    ("field", "replacement"),
    [
        ("kind", "report"),
        ("status", "FAILED"),
        ("epoch_time", "9"),
        ("error", {"code": "check_failed"}),
    ],
)
def test_system_events_rejects_failed_or_incomplete_result(
    verifier,
    tmp_path: Path,
    system_records: list[dict[str, Any]],
    field: str,
    replacement: object,
) -> None:
    system_records[-1][field] = replacement
    with pytest.raises(ValueError):
        verifier.verify_system_events(
            _write_records(tmp_path / "events.jsonl", system_records)
        )


@pytest.mark.parametrize(
    "mutation", ["missing", "duplicate", "owner", "kind", "value", "epoch"]
)
def test_system_events_rejects_invalid_final_report_gauge(
    verifier,
    tmp_path: Path,
    system_records: list[dict[str, Any]],
    mutation: str,
) -> None:
    statistics = system_records[-1]["statistics"]
    gauge = statistics[-1]
    if mutation == "missing":
        statistics.pop()
    elif mutation == "duplicate":
        statistics.append(dict(gauge))
    elif mutation == "owner":
        gauge["object_path"] = "root.counter"
    elif mutation == "kind":
        gauge["kind"] = "counter"
    elif mutation == "value":
        gauge["value"] = 2
    else:
        gauge["last_update"]["time"] = 9
    with pytest.raises(ValueError):
        verifier.verify_system_events(
            _write_records(tmp_path / "events.jsonl", system_records)
        )


class _ExportProbeCapturedError(Exception):
    pass


@pytest.fixture
def installed_export_probe(
    verifier, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> str:
    """Extract the real probe without installing a wheel or running later stages.

    Interception here is instrumentation only. The tests below execute the
    captured code with a real interpreter and check its process exit status.
    """
    captured = []

    class CaptureEnvironment:
        def __init__(self, **_kwargs):
            pass

        def create(self, _path):
            pass

    def capture(command, *, cwd):
        if command[1:3] == ["-m", "pip"]:
            return
        assert command[1] == "-c"
        assert "pycircuit.__all__" in command[2]
        captured.append(command[2])
        raise _ExportProbeCapturedError

    monkeypatch.setattr(verifier.venv, "EnvBuilder", CaptureEnvironment)
    monkeypatch.setattr(verifier, "run", capture)
    with pytest.raises(_ExportProbeCapturedError):
        verifier.installed_smoke(tmp_path, [], tmp_path)
    assert len(captured) == 1
    return captured[0]


def _run_export_probe(
    probe: str, import_root: Path, cwd: Path
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            "-I",
            "-c",
            f"import sys; sys.path.insert(0, {str(import_root)!r}); " + probe,
        ],
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )


def _source_exports() -> list[str]:
    spec = importlib.util.spec_from_file_location(
        "sdk_source_exports", ROOT / "python/pycircuit/__init__.py"
    )
    assert spec is not None and spec.loader is not None
    source = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(source)
    return source.__all__


def test_installed_export_probe_accepts_the_current_source_api(
    verifier, installed_export_probe: str, tmp_path: Path
) -> None:
    exports = _source_exports()
    assert len(exports) == len(set(exports))
    assert set(exports) == verifier.ACTIVE_EXPORTS
    result = _run_export_probe(installed_export_probe, ROOT / "python", tmp_path)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("mutation", ["missing", "extra", "duplicate", "unbound"])
def test_installed_export_probe_rejects_broken_public_bindings(
    installed_export_probe: str, tmp_path: Path, mutation: str
) -> None:
    names = list(_source_exports())
    bound = set(names)
    if mutation == "missing":
        names.remove("u37")
    elif mutation == "extra":
        names.append("CycleAwareSignal")
        bound.add("CycleAwareSignal")
    elif mutation == "duplicate":
        names.append("module")
    else:
        bound.remove("case_eq")
    (tmp_path / "pycircuit.py").write_text(
        f"__all__ = {names!r}\n"
        + "\n".join(f"{name} = object()" for name in sorted(bound))
        + "\n",
        encoding="utf-8",
    )
    result = _run_export_probe(installed_export_probe, tmp_path, tmp_path)
    assert result.returncode != 0
    assert "AssertionError" in result.stderr


@pytest.mark.parametrize("configuration", ["", "Release"])
def test_find_build_artifact_accepts_single_and_multi_configuration_layouts(
    verifier, tmp_path: Path, configuration: str
) -> None:
    runner = tmp_path / configuration / "pycircuit_sim"
    runner.parent.mkdir(parents=True, exist_ok=True)
    runner.write_bytes(b"runner fixture")
    debug_copy = tmp_path / "pycircuit_sim.dSYM/Contents/Resources/DWARF/pycircuit_sim"
    debug_copy.parent.mkdir(parents=True)
    debug_copy.write_bytes(b"debug-symbol fixture")

    assert (
        verifier.find_build_artifact(tmp_path, ("pycircuit_sim", "pycircuit_sim.exe"))
        == runner
    )


@pytest.mark.parametrize("count", [0, 2])
def test_find_build_artifact_rejects_missing_and_ambiguous_runners(
    verifier, tmp_path: Path, count: int
) -> None:
    for index in range(count):
        runner = tmp_path / str(index) / "pycircuit_sim.exe"
        runner.parent.mkdir()
        runner.write_bytes(b"runner fixture")
    with pytest.raises(ValueError, match="expected one pycircuit_sim"):
        verifier.find_build_artifact(tmp_path, ("pycircuit_sim", "pycircuit_sim.exe"))


def test_tree_snapshot_binds_relative_paths_and_exact_output_bytes(
    verifier, tmp_path: Path
) -> None:
    (tmp_path / "nested").mkdir()
    (tmp_path / "z.h").write_bytes(b"original\x00\n")
    (tmp_path / "nested/a.cpp").write_bytes(b"generated source\n")
    before = verifier.tree_snapshot(tmp_path)
    assert before == (
        ("nested/a.cpp", b"generated source\n"),
        ("z.h", b"original\x00\n"),
    )
    assert verifier.tree_snapshot(tmp_path) == before
    (tmp_path / "z.h").write_bytes(b"replacement\x00\n")
    assert verifier.tree_snapshot(tmp_path) != before
    (tmp_path / "z.h").write_bytes(b"original\x00\n")
    assert verifier.tree_snapshot(tmp_path) == before
    (tmp_path / "nested/a.cpp").unlink()
    assert verifier.tree_snapshot(tmp_path) != before
