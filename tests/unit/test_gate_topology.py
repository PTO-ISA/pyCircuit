from __future__ import annotations

import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit

FULL_CLOSURE_SCRIPTS = (
    "run_examples.sh",
    "run_api_tests.sh",
)


def _read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def test_pull_request_ci_is_python_only_and_deduplicates_api_hygiene() -> None:
    ci = _read(".github/workflows/ci.yml")

    assert "SKIP=pyc-api-hygiene pre-commit run --files" in ci
    assert ci.count("tools/check_api_hygiene.py") == 1
    assert "pytest" in ci
    assert ci.count("mkdocs build --strict") == 1

    for forbidden in (
        "llvm.sh",
        "setup-verilator",
        "setup-native-test-tools",
        "tools/pyc build",
        *FULL_CLOSURE_SCRIPTS,
    ):
        assert forbidden not in ci


def test_native_tool_bootstrap_is_confined_to_existing_linux_closure_jobs() -> None:
    action = "./.github/actions/setup-native-test-tools"
    observed = set()
    for path in sorted((ROOT / ".github/workflows").glob("*.yml")):
        workflow = yaml.safe_load(path.read_text())
        for name, job in workflow.get("jobs", {}).items():
            steps = job.get("steps", [])
            installs = [
                index for index, step in enumerate(steps) if step.get("uses") == action
            ]
            if not installs:
                continue
            observed.add((path.name, name))
            assert len(installs) == 1
            assert job["runs-on"].startswith("ubuntu-")
            python = next(
                index
                for index, step in enumerate(steps)
                if step.get("uses", "").startswith("actions/setup-python@")
            )
            build = next(
                index
                for index, step in enumerate(steps)
                if "tools/pyc build" in step.get("run", "")
            )
            assert python < installs[0] < build
            assert steps[python]["with"]["python-version"] == "3.14.6"
    assert observed == {
        ("gates-nightly.yml", "nightly-linux"),
        ("closure-probe.yml", "closure"),
        ("release.yml", "full-validation"),
    }
    ci = yaml.safe_load(_read(".github/workflows/ci.yml"))
    assert {
        step["with"]["python-version"]
        for job in ci["jobs"].values()
        for step in job["steps"]
        if step.get("uses", "").startswith("actions/setup-python@")
    } == {"3.11"}


def test_native_tool_bootstrap_checks_exact_source_identity_before_install() -> None:
    action = yaml.safe_load(_read(".github/actions/setup-native-test-tools/action.yml"))
    assert action["runs"]["using"] == "composite"
    lit, icarus = action["runs"]["steps"]
    assert lit["shell"] == icarus["shell"] == "bash"
    assert "--branch llvmorg-22.1.8" in lit["run"]
    identity = "ca7933e47d3a3451d81e72ac174dcb5aa28b59d1"
    assert lit["run"].index(identity) < lit["run"].index("python -m pip install")
    assert "git -C" in lit["run"] and "rev-parse HEAD" in lit["run"]
    assert (
        "https://github.com/steveicarus/iverilog/archive/refs/tags/v13_0.tar.gz"
        in icarus["run"]
    )
    digest = "c897bbfa9848688982c6d5c30529fc29d68df0b9ff22ffa73bad89db73a7ce49"
    assert icarus["run"].index(digest) < icarus["run"].index("tar -xzf")
    assert "sha256sum --check" in icarus["run"]
    assert '"${prefix}/bin" >> "${GITHUB_PATH}"' in icarus["run"]


def test_release_runs_each_closure_lane_and_repository_gate_once() -> None:
    release = _read(".github/workflows/release.yml")

    for script in FULL_CLOSURE_SCRIPTS:
        assert release.count(f"bash tools/{script}") == 1, script

    assert release.count("pytest tests/unit -m unit") == 1
    assert release.count("tools/check_api_hygiene.py") == 1
    assert "tools/check_decision_status.py" not in release
    assert release.count("mkdocs build --strict") == 1
    assert release.count("pre-commit run --all-files") == 1
    assert "SKIP=pyc-api-hygiene pre-commit run --all-files" in release

    assert "PYC_BUILD_TESTING=ON bash tools/pyc build" in release
    assert '--build-dir "$PWD/.pycircuit_out/toolchain/build"' in release
    assert '--install-prefix "$PWD/.pycircuit_out/toolchain/install"' in release
    assert "-DPYC_BUILD_COMPILER_DEV=ON" in _read("tools/pyc")


def test_release_build_verify_accept_publish_barriers_and_permissions() -> None:
    release = yaml.safe_load(_read(".github/workflows/release.yml"))
    jobs = release["jobs"]

    assert jobs["build-platform-candidates"]["needs"] == ["full-validation"]
    assert jobs["aggregate-candidate"]["needs"] == ["build-platform-candidates"]
    assert jobs["verify-platform-candidates"]["needs"] == ["aggregate-candidate"]
    assert set(jobs["accept-candidate"]["needs"]) == {
        "aggregate-candidate",
        "verify-platform-candidates",
    }
    assert jobs["create-tag"]["needs"] == ["accept-candidate"]
    assert jobs["publish-release"]["needs"] == ["create-tag"]
    assert jobs["publish-ghcr"]["needs"] == ["create-tag"]

    assert release["permissions"] == {"contents": "read"}
    assert jobs["create-tag"]["permissions"] == {"contents": "write"}
    assert jobs["publish-release"]["permissions"] == {
        "contents": "write",
        "id-token": "write",
    }
    assert jobs["publish-ghcr"]["permissions"] == {
        "contents": "read",
        "packages": "write",
    }
    publication_steps = json.dumps(jobs["publish-release"]["steps"])
    assert "actions/download-artifact" in publication_steps
    assert "accepted-release-${{ inputs.commit_sha }}" in publication_steps
    assert "create_wheel.py" not in publication_steps


def test_api_and_example_entrypoints_are_separate_and_tiered() -> None:
    scripts = {name: _read(f"tools/{name}") for name in FULL_CLOSURE_SCRIPTS}
    examples = scripts["run_examples.sh"]
    api = scripts["run_api_tests.sh"]
    assert "examples" in examples and "ctest" in examples
    assert "check-pycircuit" in api and "examples" not in api
    assert "tests/system/test_runtime_install.py" in api
    assert 'if [[ "$tier" == nightly ]]' in api
    for owner, content in scripts.items():
        assert "tier=gate" in content
        assert "--tier)" in content and "--list)" in content
        assert "pyc_list_tests" in content
        for nested in FULL_CLOSURE_SCRIPTS:
            if nested != owner:
                assert nested not in content
        assert "pycircuit-backend-test" not in content
    for retired in (
        "run_sims.sh",
        "run_sims_nightly.sh",
        "run_semantic_regressions_v6.sh",
    ):
        assert not (ROOT / "tools" / retired).exists()
    for workflow in ("release.yml", "gates-nightly.yml"):
        text = _read(f".github/workflows/{workflow}")
        for script in FULL_CLOSURE_SCRIPTS:
            assert text.count(f"bash tools/{script} --tier nightly") == 1


@pytest.mark.parametrize("script", FULL_CLOSURE_SCRIPTS)
def test_entrypoints_reject_invalid_selection_before_toolchain_access(
    script: str,
) -> None:
    import subprocess

    result = subprocess.run(
        ["bash", str(ROOT / "tools" / script), "--tier", "typo"],
        capture_output=True,
        text=True,
        timeout=5,
    )
    assert result.returncode != 0
    assert "tier must be gate or nightly" in result.stderr


def _configure_ctest_project(tmp_path: Path, body: str) -> Path:
    """Exercise CMake/CTest metadata without creating or building executables."""
    for tool in ("cmake", "ctest"):
        if shutil.which(tool) is None:
            pytest.skip(f"{tool} is required for the real CTest topology probe")
    source = tmp_path / "source"
    build = tmp_path / "build"
    source.mkdir()
    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\n"
        "project(GateTopology NONE)\n"
        "enable_testing()\n" + body,
        encoding="utf-8",
    )
    result = subprocess.run(
        ["cmake", "-S", str(source), "-B", str(build)],
        capture_output=True,
        text=True,
        timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return build


@pytest.mark.parametrize("explicit_tiers", [True, False], ids=["gate", "standalone"])
def test_deferred_example_labels_include_later_auxiliary_tests(
    tmp_path: Path,
    explicit_tiers: bool,
) -> None:
    labels = (
        'set(PYC_EXAMPLE_LABELS "examples;gate;nightly")\n' if explicit_tiers else ""
    )
    build = _configure_ctest_project(
        tmp_path,
        f'include("{(ROOT / "cmake/PycircuitExamples.cmake").as_posix()}")\n'
        + labels
        + 'add_test(NAME main COMMAND "${CMAKE_COMMAND}" -E true)\n'
        "cmake_language(DEFER CALL pycircuit_label_example_tests)\n"
        'add_test(NAME later_auxiliary COMMAND "${CMAKE_COMMAND}" -E true)\n',
    )
    result = subprocess.run(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"],
        capture_output=True,
        text=True,
        timeout=10,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    tests = json.loads(result.stdout)["tests"]
    assert {test["name"] for test in tests} == {"main", "later_auxiliary"}
    expected = {"examples", "nightly"} | ({"gate"} if explicit_tiers else set())
    for test in tests:
        properties = {item["name"]: item["value"] for item in test["properties"]}
        assert set(properties["LABELS"]) == expected, test


def test_list_tests_rejects_empty_real_ctest_selection(tmp_path: Path) -> None:
    build = _configure_ctest_project(
        tmp_path,
        'add_test(NAME api_gate COMMAND "${CMAKE_COMMAND}" -E true)\n'
        'set_tests_properties(api_gate PROPERTIES LABELS "api;gate")\n',
    )
    command = [
        "bash",
        "-c",
        'source "$1"; pyc_list_tests api "$2" "$3"',
        "gate-topology",
        str(ROOT / "tools/lib.sh"),
    ]
    env = {**os.environ, "PYC_PYTHON_EXECUTABLE": sys.executable}
    selected = subprocess.run(
        [*command, "gate", str(build)],
        env=env,
        capture_output=True,
        text=True,
        timeout=10,
    )
    assert selected.returncode == 0, selected.stdout + selected.stderr
    assert [test["name"] for test in json.loads(selected.stdout)["tests"]] == [
        "api_gate"
    ]
    empty = subprocess.run(
        [*command, "nightly", str(build)],
        env=env,
        capture_output=True,
        text=True,
        timeout=10,
    )
    assert empty.returncode != 0
    assert "empty CTest selection" in empty.stderr


def test_api_nightly_keeps_all_current_public_flow_oracles() -> None:
    api = _read("tools/run_api_tests.sh")
    declaration = re.search(r"nightly_python_tests=\((.*?)\)", api, re.DOTALL)
    assert declaration is not None
    listed = re.findall(r"tests/system/test_[a-z_]+\.py", declaration.group(1))
    expected = {
        f"tests/system/test_{name}.py"
        for name in (
            "source_map",
            "public_emit",
            "incremental_build",
            "relocated_compiler",
            "publication_process_recovery",
            "runtime_install",
            "cmake_presets",
        )
    }
    assert expected <= set(listed)
    assert all(listed.count(path) == 1 for path in expected)
    branches = re.findall(
        r'if \[\[ "\$tier" == nightly \]\]; then(.*?)\nfi', api, re.DOTALL
    )
    assert any(
        '-m pytest -q "${nightly_python_tests[@]}"' in branch for branch in branches
    )


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
    """The release closure calls the current source compiler retirement gate."""
    release = _read(".github/workflows/release.yml")
    assert "tools/check_frontend_retirement.py" in release
    assert "run_agentic_circuit.sh" not in release


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
    for script in sorted((ROOT / "tools").glob("*.sh")):
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
    """Each platform wheel carries the sole pyCircuit distribution and driver."""

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
    assert 'name="pycircuit-hisi"' in setup
    assert '"pycircuit=pycircuit.cli:main"' in setup
    assert "acc.py=" not in setup
    assert "agentic-circuit=" not in setup

    builder = _read("packaging/wheel/create_wheel.py")
    assert "_stage_installed_python(install_dir, package_dir)" in builder
    assert "_drop_bundled_python_copy(package_dir)" in builder
    assert '"pycircuit-source-unit"' in builder
    assert '"pycircuit-link"' in builder
    assert '"pycircuit-emit"' in builder


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
    assert "_drop_bundled_python_copy(package_dir)" in builder

    verifier = _read("packaging/sdk/verify_platform_candidate.py")
    assert 'driver = commands / f"pycircuit{suffix}"' in verifier
    assert (
        'for name in ("pycc", "pyc-opt", "acc", "acc.py", "agentic-circuit")'
        in verifier
    )
    assert 'raise ValueError("installed wheel is missing pycircuit")' in verifier
    assert '"compile",' in verifier
    assert '"link", unit' in verifier
    assert 'command = [driver, "emit"' in verifier

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
