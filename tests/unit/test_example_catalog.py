"""Validate catalog publication; synthetic artifacts do not execute hardware."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture
def tool():
    spec = importlib.util.spec_from_file_location(
        "example_catalog_under_test", ROOT / "tools/pycircuit/example_catalog.py"
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value) + "\n")


def _hash(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


@pytest.fixture
def catalog_repo(tmp_path: Path):
    rows = [
        {
            "name": name,
            "folder": f"examples/{name}",
            "source": f"{name}.py",
            **({"generated": "GENERATED.json"} if generated else {}),
        }
        for name, generated in (("alpha", False), ("beta", True))
    ]
    api_rows = [{"name": "memory_case", "owner": "tests/compiler/lit/memory.py"}]
    data = {
        "schema": "pycircuit-example-catalog",
        "examples": rows,
        "api_coverage": api_rows,
    }
    _write_json(tmp_path / "examples/catalog.json", data)
    for row in rows:
        folder = tmp_path / row["folder"]
        folder.mkdir(parents=True)
        for name in ("CMakeLists.txt", "config.json", row["source"]):
            (folder / name).write_text("fixture\n")
        if row.get("generated"):
            (folder / row["generated"]).write_text("{}\n")
            for name in ("README.md", "driver.cpp", "rtl_tb.sv", "GENERATED.md"):
                (folder / name).write_text("documentation fixture\n")
    owner = tmp_path / api_rows[0]["owner"]
    owner.parent.mkdir(parents=True)
    owner.write_text("# API coverage fixture\n")
    return tmp_path, data


def test_navigation_lists_current_examples_and_api_owners(tool, catalog_repo):
    repo, data = catalog_repo
    assert tool.catalog(repo)["examples"] == data["examples"]
    text = tool.navigation(repo)
    assert "**2** runnable examples" in text
    assert "**1** API-owned coverage cases" in text
    assert "| alpha | [Python](alpha/alpha.py) | — | — | — |" in text
    assert "[beta](beta/README.md)" in text
    assert "[driver](beta/driver.cpp)" in text
    assert "[testbench](beta/rtl_tb.sv)" in text
    assert "[artifacts](beta/GENERATED.md)" in text
    assert "| memory_case | [test](../tests/compiler/lit/memory.py) |" in text


def test_catalog_rejects_incomplete_example_coverage(tool, catalog_repo):
    repo, data = catalog_repo
    (repo / data["examples"][0]["folder"] / "alpha.py").unlink()
    with pytest.raises(ValueError, match="example coverage is incomplete: alpha"):
        tool.catalog(repo)


def test_catalog_rejects_missing_api_owner(tool, catalog_repo):
    repo, data = catalog_repo
    (repo / data["api_coverage"][0]["owner"]).unlink()
    with pytest.raises(ValueError, match="API coverage owner is missing: memory_case"):
        tool.catalog(repo)


def test_catalog_rejects_duplicate_names(tool, catalog_repo):
    repo, data = catalog_repo
    data["api_coverage"][0]["name"] = data["examples"][0]["name"]
    _write_json(repo / tool.CATALOG, data)
    with pytest.raises(ValueError, match="duplicate names"):
        tool.catalog(repo)


def test_checked_out_catalog_matches_registered_examples(tool):
    data = tool.catalog(ROOT)
    cmake = (ROOT / "examples/CMakeLists.txt").read_text()
    registered = [
        line.removeprefix("add_selected_example(").removesuffix(")")
        for line in cmake.splitlines()
        if line.startswith("add_selected_example(")
    ]
    assert [row["name"] for row in data["examples"]] == registered
    navigation = tool.navigation(ROOT)
    assert "docs/gates/logs" not in navigation
    assert "migration" not in navigation.lower()


@pytest.fixture
def documented_build(tmp_path: Path):
    """Make valid receipt-shaped data solely for documentation validation."""
    repo = tmp_path / "repo"
    source = repo / "examples/fixture"
    source.mkdir(parents=True)
    (source / "fixture.py").write_text("# source fixture\n")
    (source / "config.json").write_text("{}\n")
    build = tmp_path / "build"
    build.mkdir()
    include = tmp_path / "runtime-include"
    (include / "verilog").mkdir(parents=True)
    (include / "verilog/primitive.v").write_text("module primitive; endmodule\n")
    runner = tmp_path / "runner"
    runner.write_text("synthetic runner identity, never executed\n")
    payloads = {
        "fixture.ac": ('"ac.module"', "mlir"),
        "cpp/fixture.hpp": ("void Work()", "cpp"),
        "verilog/fixture.v": ("module fixture", "rtl"),
    }
    for relative, (marker, label) in payloads.items():
        path = build / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            "preamble\n"
            + marker
            + "\n"
            + "\n".join(f"{label}_literal_{i}" for i in range(20))
            + "\n"
        )
    _write_json(
        build / "cpp/generated.json",
        {
            "entry": "fixture.Root",
            "entry_source": {"path": "fixture.py"},
            "files": [{"path": "fixture.hpp", "role": "header"}],
        },
    )
    _write_json(
        build / "verilog/generated.json",
        {
            "files": [{"path": "fixture.v", "role": "rtl"}],
        },
    )
    _write_json(
        build / "execution.json",
        [
            {"command": [str(runner), "--workers", "1"], "exit_status": 0},
            {"command": [str(runner), "--workers", "2"], "exit_status": 0},
            {"command": ["verilator", "--binary"], "exit_status": 0},
            {"command": ["compiled-rtl-fixture"], "exit_status": 0},
        ],
    )
    trace = "WORK fixture 0\nWORK fixture 1\n"
    for filename in ("serial.stdout", "parallel.stdout", "rtl-run.stdout"):
        (build / filename).write_text("non-Work output\n" + trace)
    inputs = {"runner": _hash(runner)}
    for filename in ("fixture.py", "config.json"):
        inputs[f"source/{filename}"] = _hash(source / filename)
    for relative in (*payloads, "cpp/generated.json", "verilog/generated.json"):
        inputs[f"build/{relative}"] = _hash(build / relative)
    inputs["runtime-rtl/primitive.v"] = _hash(include / "verilog/primitive.v")
    proof = {
        "schema": "pycircuit-example-verification-v1",
        "inputs": inputs,
        "execution_sha256": _hash(build / "execution.json"),
        "work_samples": 2,
        "workers": [1, 2],
        "rtl": "verilator",
        "trace_sha256": hashlib.sha256(trace.encode()).hexdigest(),
        "runtime_include": str(include),
    }
    _write_json(build / "verification.json", proof)
    return repo, build, source, runner


def test_generated_guide_copies_literal_artifact_excerpts(tool, documented_build):
    repo, build, _, _ = documented_build
    text, metadata = tool.generated(repo, "fixture", build)
    assert metadata["example"] == "fixture"
    assert metadata["entry"] == "fixture.Root"
    proof = json.loads((build / "verification.json").read_text())
    assert metadata["verification"] == {
        key: value for key, value in proof.items() if key != "runtime_include"
    }
    assert "runtime_include" not in metadata["verification"]
    assert {row["path"] for row in metadata["excerpts"]} == {
        "fixture.ac",
        "cpp/fixture.hpp",
        "verilog/fixture.v",
    }
    for row in metadata["excerpts"]:
        path = build / row["path"]
        assert row["line"] == 2
        assert row["sha256"] == _hash(path)
        literal = "\n".join(
            path.read_text().splitlines()[row["line"] - 1 : row["line"] + 13]
        )
        assert literal in text
    assert "mlir_literal_19" not in text


@pytest.mark.parametrize("changed", ["source", "artifact", "runner"])
def test_generated_guide_refuses_stale_verified_inputs(tool, documented_build, changed):
    repo, build, source, runner = documented_build
    path, identity = {
        "source": (source / "fixture.py", "source/fixture.py"),
        "artifact": (build / "cpp/fixture.hpp", "build/cpp/fixture.hpp"),
        "runner": (runner, "runner"),
    }[changed]
    path.write_text(path.read_text() + "changed after verification\n")
    with pytest.raises(ValueError, match=f"stale verified example input: {identity}"):
        tool.generated(repo, "fixture", build)


def test_generated_guide_refuses_source_added_after_verification(
    tool, documented_build
):
    repo, build, source, _ = documented_build
    (source / "another_module.py").write_text("# new source after the recorded run\n")
    with pytest.raises(
        ValueError, match="(source|input).*(changed|membership|inventory|set)"
    ):
        tool.generated(repo, "fixture", build)


@pytest.mark.parametrize("change", ["changed", "added"])
def test_generated_guide_refuses_changed_runtime_rtl(tool, documented_build, change):
    repo, build, _, _ = documented_build
    proof = json.loads((build / "verification.json").read_text())
    runtime = Path(proof["runtime_include"]) / "verilog"
    path = runtime / ("primitive.v" if change == "changed" else "new_primitive.v")
    path.write_text("module altered; endmodule\n")
    gate = (
        "runtime-rtl/primitive.v"
        if change == "changed"
        else "(input|runtime).*(set|changed|membership|inventory)"
    )
    with pytest.raises(ValueError, match=gate):
        tool.generated(repo, "fixture", build)


def test_generated_guide_refuses_failed_execution_even_with_matching_digest(
    tool, documented_build
):
    repo, build, _, _ = documented_build
    commands = json.loads((build / "execution.json").read_text())
    commands[1]["exit_status"] = 17
    _write_json(build / "execution.json", commands)
    proof = json.loads((build / "verification.json").read_text())
    proof["execution_sha256"] = _hash(build / "execution.json")
    _write_json(build / "verification.json", proof)
    with pytest.raises(ValueError, match="execution is missing or failed"):
        tool.generated(repo, "fixture", build)


def test_generated_guide_refuses_changed_execution_receipt(tool, documented_build):
    repo, build, _, _ = documented_build
    commands = json.loads((build / "execution.json").read_text())
    commands[0]["command"].append("--changed-command")
    _write_json(build / "execution.json", commands)
    with pytest.raises(ValueError, match="execution changed after verification"):
        tool.generated(repo, "fixture", build)


@pytest.mark.parametrize("change", ["disagree", "all_changed", "all_empty"])
def test_generated_guide_refuses_invalid_work_traces(tool, documented_build, change):
    repo, build, _, _ = documented_build
    filenames = (
        ("parallel.stdout",)
        if change == "disagree"
        else (
            "serial.stdout",
            "parallel.stdout",
            "rtl-run.stdout",
        )
    )
    for filename in filenames:
        (build / filename).write_text(
            "noise\n"
            if change == "all_empty"
            else "WORK different 0\nWORK different 1\n"
        )
    gate = (
        "Work trace changed after verification"
        if change == "all_changed"
        else "verified Work traces are missing or disagree"
    )
    with pytest.raises(ValueError, match=gate):
        tool.generated(repo, "fixture", build)


def test_generated_guide_requires_current_successful_receipt(tool, documented_build):
    repo, build, _, _ = documented_build
    proof = json.loads((build / "verification.json").read_text())
    proof["schema"] = "obsolete-receipt"
    _write_json(build / "verification.json", proof)
    with pytest.raises(ValueError, match="current successful verification receipt"):
        tool.generated(repo, "fixture", build)


def test_verifier_removes_previous_success_before_real_failed_process(documented_build):
    """Exercise subprocess failure bookkeeping, not a DUT or RTL oracle."""
    _, build, source, _ = documented_build
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "cmake/verify_example.py"),
            "--runner",
            sys.executable,
            "--build",
            str(build),
            "--include",
            str(source),
            "--verilator",
            "not-reached",
            "--source",
            str(source),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    # Python rejects runner-only --config, so the actually launched process fails.
    assert result.returncode != 0
    assert "serial:" in result.stderr
    assert not (build / "verification.json").exists()
    commands = json.loads((build / "execution.json").read_text())
    assert len(commands) == 1
    assert Path(commands[0]["command"][0]).resolve() == Path(sys.executable).resolve()
    assert commands[0]["exit_status"] != 0
