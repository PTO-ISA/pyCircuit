from __future__ import annotations

import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

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
    # The pyc dialect suite holds the family/attribute contracts and used to run
    # nowhere in CI.
    assert "--target check-pyc" in agentic
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


def test_lanes_running_check_acir_install_ripgrep() -> None:
    """The source-hygiene lit test shells out to `rg`.

    It fails closed when ripgrep is missing, so every lane that runs check-acir
    must install it; the release lane shipped without it and the whole closure
    failed on one source-hygiene test.
    """

    consumers = [
        path
        for path in sorted((ROOT / "tests/mlir").rglob("*.mlir"))
        if re.search(r"RUN:.*%not\s+rg\b", path.read_text(encoding="utf-8"))
    ]
    assert consumers, "no lit test depends on ripgrep"

    lanes = [
        path
        for path in sorted((ROOT / ".github/workflows").glob("*.yml"))
        if "run_agentic_circuit.sh" in path.read_text(encoding="utf-8")
    ]
    assert lanes, "no workflow runs the Agentic Circuit closure"
    for lane in lanes:
        assert "ripgrep" in lane.read_text(encoding="utf-8"), lane.name


def test_release_attestation_commands_are_repo_explicit() -> None:
    """The final attestation job runs without a checkout.

    GitHub CLI resolves `--repo` on its own; without it the v6.0.0 attestation
    job failed with "not a git repository" and the release lost its verification
    link.
    """

    release = _read(".github/workflows/release.yml")
    for command in ("gh release view", "gh release edit", "gh issue comment"):
        matches = [line for line in release.splitlines() if command in line]
        assert matches, command
        for line in matches:
            assert "--repo" in line, line


def test_closure_scripts_do_not_pipe_python_into_an_early_exit_consumer() -> None:
    """A consumer that stops reading closes the pipe under the producer.

    `run_semantic_regressions_v6.sh` filtered a Python producer through
    `awk ... exit`; the producer then raised BrokenPipeError while flushing
    stdout at shutdown, exit code 120, and `pipefail` failed the whole gate.
    """

    offenders: list[str] = []
    for script in sorted((ROOT / "flows/scripts").glob("*.sh")):
        for number, line in enumerate(
            script.read_text(encoding="utf-8").splitlines(), 1
        ):
            if "python" not in line or "|" not in line:
                continue
            producer, consumer = line.split("|", 1)
            if "python" not in producer:
                continue
            if re.search(r"\bexit\b", consumer) or re.search(r"\bhead\b", consumer):
                offenders.append(f"{script.name}:{number}")

    assert offenders == []


def test_pypi_publication_cannot_invalidate_a_published_release() -> None:
    """The package host is outside the language contract.

    Publication runs as its own workflow over the already published release
    bytes, and the release workflow contains no upload at all: a rejected upload
    must not leave an already published GitHub release unattested, and an
    already cut release must be publishable to PyPI later without rebuilding,
    re-tagging, or re-verifying anything.
    """

    release = yaml.safe_load(_read(".github/workflows/release.yml"))
    jobs = release["jobs"]
    assert "pypa/gh-action-pypi-publish" not in json.dumps(release)
    assert "PYC_PUBLISH_PYPI" not in json.dumps(release)
    for placeholder in ("publish-pypi", "pypi"):
        assert placeholder not in jobs, jobs

    text = _read(".github/workflows/publish-pypi.yml")
    publication = yaml.safe_load(text)
    triggers = publication[True] if True in publication else publication["on"]
    assert list(triggers) == ["workflow_dispatch"], triggers
    inputs = triggers["workflow_dispatch"]["inputs"]
    assert set(inputs) == {"version", "source_revision", "max_upload_bytes"}
    assert inputs["version"]["required"] is True
    assert inputs["source_revision"]["required"] is True
    assert inputs["max_upload_bytes"]["required"] is False
    assert inputs["max_upload_bytes"]["default"] == "104857600"

    (job,) = publication["jobs"].values()
    assert job["environment"] == {"name": "release"}
    assert job["permissions"]["id-token"] == "write"
    assert job["permissions"]["contents"] == "read"

    assert "pypa/gh-action-pypi-publish@" in text
    # The upload must be the release bytes, never a rebuild.
    for rebuild in (
        "cmake --build",
        "python3 -m build",
        "create_wheel.py",
        "pyc build",
    ):
        assert rebuild not in text, rebuild
    assert "gh release download" in text
    # The tag is the source revision pin, and the selection is version-scoped so
    # only the wheels the release itself carries are ever published.
    assert "refs/tags/v${version}^{}" in text
    assert "${{ inputs.source_revision }}" in text
    assert "len(selected) != 3" in text
    assert "len(platforms) != 3" in text
    assert 'expected = {"pycircuit-hisi"}' in text
    # The host rejects a wheel over its per-file limit, so a release whose Linux
    # wheel exceeds it must publish what fits and name what was deferred instead
    # of failing the whole upload.
    assert "max_upload_bytes" in text
    assert "Deferred:" in text
    assert "exceed the {limit}-byte host " in text
    # The host is irreversible, so an interrupted upload must stay recoverable
    # instead of failing forever on the files it already accepted.
    (upload,) = [
        step
        for step in job["steps"]
        if "pypa/gh-action-pypi-publish" in json.dumps(step)
    ]
    assert upload["with"] == {
        "packages-dir": "wheels",
        "password": "${{ secrets.PYPI_API_TOKEN }}",
        "skip-existing": True,
    }

    def needs_closure(name: str) -> set[str]:
        seen: set[str] = set()
        pending = list(jobs[name].get("needs") or [])
        while pending:
            current = pending.pop()
            if current in seen:
                continue
            seen.add(current)
            pending.extend(jobs[current].get("needs") or [])
        return seen

    for dependent in (
        "verify-published-bytes",
        "verify-stable-platforms",
        "release-attestation",
    ):
        assert not needs_closure(dependent) & set(publication["jobs"]), dependent


def test_release_ships_exactly_one_wheel_per_platform() -> None:
    """One wheel carries both frontends and both compilers.

    `_pycircuit_semantics` and `agentic_circuit` are staged inside the platform
    wheel, so no lane may build, publish, or depend on a second distribution:
    a universal-wheel lane would put them back on the package host as separate
    projects and let a consumer install a wheel without the native bridge.
    """

    release = _read(".github/workflows/release.yml")
    evidence = _read(".github/workflows/platform-evidence.yml")
    assert "build-universal-wheels" not in release
    for text in (release, evidence):
        for retired in (
            "release-shard-universal",
            "platform-evidence-universal",
            "python3 -m build --wheel",
            "find universal -name",
            "universalWheel",
        ):
            assert retired not in text, retired

    version_map = json.loads(_read("packaging/sdk/version-map.json"))
    assert version_map["distributions"] == {
        "pycircuit-hisi": version_map["product_version"]
    }

    setup = _read("packaging/wheel/setup.py")
    assert 'VENDORED_PACKAGES = ("_pycircuit_semantics", "agentic_circuit")' in setup
    assert "pycircuit-semantic-core" not in setup
    for script in (
        "acc=pycircuit.packaged_toolchain:acc_main",
        "acc.py=",
        "agentic-circuit=",
    ):
        assert script in setup, script

    builder = _read("packaging/wheel/create_wheel.py")
    assert "_stage_vendored_packages(install_dir, stage)" in builder
    assert "NATIVE_EXTENSION" in builder


def test_wheel_build_relocates_bundled_libraries() -> None:
    """A wheel must not reference the builder's absolute toolchain paths.

    The build links the platform's own LLVM, z3, and zstd, and those references
    are absolute on macOS. A wheel that shipped them verbatim would only start on
    a machine with the builder's exact Homebrew tree, which is how the published
    `pycc` failed platform verification. The SDK archive builder already
    relocates its copy, so the wheel build must call that same implementation,
    and every lane that builds the wheel must state the profile it relocates for.
    """

    builder = _read("packaging/wheel/create_wheel.py")
    assert "import create_platform_manifest" in builder
    assert "create_platform_manifest.relocate_native_dependencies(" in builder
    assert 'stage / "pycircuit" / "_toolchain" / "lib"' in builder
    assert "_relocate(stage, args.platform or _platform_for(plat_name))" in builder
    assert "_drop_toolchain_frontend_copies(package_dir)" in builder

    verifier = _read("packaging/sdk/verify_platform_candidate.py")
    assert 'for name in ("pycc", "acc"):' in verifier
    assert "compiler_failure_report(" in verifier
    for outcome in (
        "installed wheel console script",
        "SDK tree binary",
        "installed wheel binary",
    ):
        assert outcome in verifier, outcome

    for workflow in (
        ".github/workflows/release.yml",
        ".github/workflows/platform-evidence.yml",
    ):
        text = _read(workflow)
        assert (
            text.count(
                '--wheel-version "${{ inputs.version }}" '
                '--platform "${{ matrix.platform }}"'
            )
            == 2
        ), workflow
