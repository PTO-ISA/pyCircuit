from __future__ import annotations

import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit

FULL_CLOSURE_SCRIPTS = (
    "run_agentic_circuit.sh",
    "run_examples.sh",
    "run_sims.sh",
    "run_sims_nightly.sh",
    "run_semantic_regressions_v6.sh",
)


def _read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def test_pull_request_ci_is_python_only_and_deduplicates_api_hygiene() -> None:
    ci = _read(".github/workflows/ci.yml")

    assert "SKIP=pyc-api-hygiene pre-commit run --files" in ci
    assert ci.count("flows/tools/check_api_hygiene.py") == 1
    assert ci.count("pytest tests/unit -m unit") == 1
    assert ci.count("mkdocs build --strict") == 1

    for forbidden in (
        "llvm.sh",
        "setup-verilator",
        "flows/scripts/pyc build",
        *FULL_CLOSURE_SCRIPTS,
    ):
        assert forbidden not in ci


def test_release_runs_each_closure_lane_and_repository_gate_once() -> None:
    release = _read(".github/workflows/release.yml")

    for script in FULL_CLOSURE_SCRIPTS:
        assert release.count(f"bash flows/scripts/{script}") == 1, script

    assert release.count("pytest tests/unit -m unit") == 1
    assert release.count("flows/tools/check_api_hygiene.py") == 1
    assert release.count("flows/tools/check_decision_status.py") == 1
    assert release.count("mkdocs build --strict") == 1
    assert release.count("pre-commit run --all-files") == 1
    assert "SKIP=pyc-api-hygiene pre-commit run --all-files" in release

    assert "PYC_BUILD_AGENTIC_CIRCUIT_TESTS=ON" in release
    assert 'AC_GATE_BUILD_ROOT="$PWD/.pycircuit_out/toolchain/build"' in release
    assert (
        'cmake --build "$PWD/.pycircuit_out/toolchain/build" --target check-acir'
        not in release
    )
    assert 'ctest --test-dir "$PWD/.pycircuit_out/toolchain/build"' not in release


def test_closure_scripts_are_composable_and_partition_simulation_coverage() -> None:
    scripts = {name: _read(f"flows/scripts/{name}") for name in FULL_CLOSURE_SCRIPTS}
    agentic = scripts["run_agentic_circuit.sh"]
    examples = scripts["run_examples.sh"]
    sims = scripts["run_sims.sh"]
    nightly = scripts["run_sims_nightly.sh"]
    semantic = scripts["run_semantic_regressions_v6.sh"]

    for root_gate in (
        "pytest tests/unit",
        "check_api_hygiene.py",
        "check_decision_status.py",
        "mkdocs build",
    ):
        assert root_gate not in agentic
        assert root_gate not in examples

    for owner, content in scripts.items():
        for nested in FULL_CLOSURE_SCRIPTS:
            if nested != owner:
                assert nested not in content, f"{owner} invokes {nested}"

    assert "tools/agentic-circuit/check-contracts.py" in agentic
    assert "tests/python/agentic-circuit/contracts" in agentic
    assert "--target check-acir" in agentic
    assert "ctest --test-dir" in agentic

    semantic_cases = {
        "net_resolution_depth_smoke",
        "reset_invalidate_order_smoke",
        "xz_value_model_smoke",
    }
    assert "--tier normal" in sims
    for semantic_case in semantic_cases:
        assert semantic_case in sims
        assert f'run_case "{semantic_case}"' in semantic
    assert (
        "net_resolution_depth_smoke|reset_invalidate_order_smoke|"
        "xz_value_model_smoke) continue ;;"
    ) in sims
    assert "--tier heavy" in nightly
    assert "--tier all" not in nightly
    assert "fixtures/bypass_unit" in nightly
    assert "fixtures/issq" not in nightly
    assert "fixtures/regfile" not in nightly

    discovered = subprocess.run(
        (
            "python3",
            "flows/tools/discover_examples.py",
            "--root",
            "examples/pycircuit",
            "--tier",
            "all",
            "--format",
            "json",
        ),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    )
    records = json.loads(discovered.stdout)
    all_examples = {record["name"] for record in records}
    normal = {
        record["name"] for record in records if record["tier"] == "normal"
    } - semantic_cases
    heavy = {record["name"] for record in records if record["tier"] == "heavy"}

    assert normal.isdisjoint(heavy)
    assert normal.isdisjoint(semantic_cases)
    assert heavy.isdisjoint(semantic_cases)
    assert normal | heavy | semantic_cases == all_examples


def _workflow_env(text: str, name: str) -> str:
    match = re.search(rf'^\s*{name}:\s*"([^"]*)"\s*$', text, re.MULTILINE)
    assert match is not None, f"{name} is not pinned in the workflow"
    return match.group(1)


def test_release_and_evidence_windows_lanes_pin_the_same_clang_cl_driver() -> None:
    release = _read(".github/workflows/release.yml")
    evidence = _read(".github/workflows/platform-evidence.yml")

    for workflow in (release, evidence):
        version = _workflow_env(workflow, "CLANG_PACKAGE_VERSION")
        url = _workflow_env(workflow, "CLANG_SOURCE_URL")

        assert version in url
        assert "x86_64-pc-windows-msvc" in url
        # clang-cl is the Windows compiler. cl.exe cannot build the ACIR
        # codegen: its front end aborts with C1001 on the recursive generic
        # lambdas that clang accepts.
        assert '$env:CC = "clang-cl.exe"' in workflow
        assert '$env:CXX = "clang-cl.exe"' in workflow
        assert '$env:CC = "cl.exe"' not in workflow
        # PowerShell's -notmatch returns the NON-matching elements when it is
        # handed an array, so the banner must be joined before it is negated.
        assert "$clangInfo = (& clang-cl.exe --version) -join" in workflow
        # clang-cl still reads INCLUDE, LIB, and link.exe from the MSVC
        # developer environment.
        assert "ilammy/msvc-dev-cmd@" in workflow
        # clang-cl has no -ffile-prefix-map spelling of its own and ignores
        # it, so the mapping must go through the /clang: driver escape.
        assert "/clang:-ffile-prefix-map=" in workflow
        assert 'CFLAGS = "-ffile-prefix-map=' not in workflow

    # The evidence lane must reproduce the release lane exactly.
    assert _workflow_env(release, "CLANG_PACKAGE_VERSION") == _workflow_env(
        evidence, "CLANG_PACKAGE_VERSION"
    )
    assert _workflow_env(release, "CLANG_SOURCE_URL") == _workflow_env(
        evidence, "CLANG_SOURCE_URL"
    )


def test_windows_platform_record_probes_the_compiler_version_and_keeps_msvc_abi() -> (
    None
):
    spec = importlib.util.spec_from_file_location(
        "create_platform_manifest",
        ROOT / "packaging/sdk/create_platform_manifest.py",
    )
    assert spec is not None and spec.loader is not None
    generator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(generator)

    # clang-cl answers --version, so the probe must use that path rather than
    # the cl.exe banner fallback. Any interpreter exercises the same branch.
    record = generator.platform_record("windows-x86_64", sys.executable)

    assert record["id"] == "windows-x86_64"
    assert record["host_triple"] == "x86_64-pc-windows-msvc"
    assert record["cxx_abi"] == "MSVC v143"
    assert record["cxx_compiler"].startswith("Python ")


def test_windows_manifest_steps_check_native_exit_codes() -> None:
    """PowerShell masks failing native commands unless they are checked.

    $ErrorActionPreference does not apply to native commands, so a failing
    create_platform_manifest.py run let the lane upload a candidate with only
    the wheel in it; the loss surfaced much later as a missing manifest in the
    verify job. Every native packaging step must check $LASTEXITCODE.
    """
    for name in (
        ".github/workflows/release.yml",
        ".github/workflows/platform-evidence.yml",
    ):
        workflow = _read(name)
        assert 'if ($LASTEXITCODE -ne 0) { throw "wheel creation failed' in workflow
        assert (
            'if ($LASTEXITCODE -ne 0) { throw "platform manifest and archive creation failed'
            in workflow
        )
        assert 'if ($LASTEXITCODE -ne 0) { throw "twine check failed' in workflow
