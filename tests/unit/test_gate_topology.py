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


def test_native_tool_bootstrap_precedes_build_in_linux_jobs() -> None:
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
                if any(
                    command in step.get("run", "")
                    for command in ("tools/pyc build", "cmake --build")
                )
            )
            assert python < installs[0] < build
    assert observed == {
        ("ci.yml", "native-transactional-gates"),
        ("gates-nightly.yml", "nightly-linux"),
        ("closure-probe.yml", "closure"),
        ("release.yml", "full-validation"),
    }


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


@pytest.mark.parametrize(
    "build_status,native_status,expected_steps,success",
    [
        (0, 0, ["build", "native", "python"], True),
        (0, 1, ["build", "native", "python"], False),
        (1, 0, ["build"], False),
    ],
    ids=["success", "native-failure-continues", "build-failure-stops"],
)
def test_api_test_group_scheduling(
    tmp_path: Path,
    build_status: int,
    native_status: int,
    expected_steps: list[str],
    success: bool,
) -> None:
    """Execute the entrypoint with controlled build and test outcomes."""
    tools = tmp_path / "tools"
    tools.mkdir()
    for name in ("run_api_tests.sh", "lib.sh"):
        shutil.copy2(ROOT / "tools" / name, tools / name)
    build = tmp_path / "build"
    build.mkdir()
    (build / "CMakeCache.txt").touch()
    binaries = tmp_path / "install" / "bin"
    binaries.mkdir(parents=True)
    for name in ("pycircuit-source-unit", "pycircuit-link", "pycircuit-emit"):
        helper = binaries / name
        helper.write_text("#!/bin/sh\nexit 0\n")
        helper.chmod(0o755)
    record = tmp_path / "steps.jsonl"
    stub = f"""#!{sys.executable}
import json
import os
import sys
from pathlib import Path
name = Path(sys.argv[0]).name
args = sys.argv[1:]
def record(step):
    with open(os.environ['STEP_RECORD'], 'a') as stream:
        stream.write(json.dumps(step) + '\\n')
if name == 'cmake':
    if '--build' in args:
        record('build')
        sys.exit(int(os.environ['BUILD_STATUS']))
elif name == 'ctest':
    if '--show-only=json-v1' in args:
        print(json.dumps({{'tests': [{{'name': 'native'}}]}}))
    else:
        record('native')
        Path(args[args.index('--output-junit') + 1]).write_text('<testsuite/>')
        sys.exit(int(os.environ['NATIVE_STATUS']))
elif args[:2] == ['-m', 'pytest']:
    record('python')
    junit = next(arg.split('=', 1)[1] for arg in args if arg.startswith('--junitxml='))
    Path(junit).write_text('<testsuite/>')
else:
    os.execv({sys.executable!r}, [{sys.executable!r}, *args])
"""
    for name in ("cmake", "ctest", "python-test"):
        command = binaries / name
        command.write_text(stub)
        command.chmod(0o755)
    env = {
        **os.environ,
        "PATH": f"{binaries}:{os.environ['PATH']}",
        "PYC_BUILD_DIR": str(build),
        "PYC_TOOLCHAIN_ROOT": str(binaries.parent),
        "PYC_PYTHON_EXECUTABLE": str(binaries / "python-test"),
        "PYC_GATE_RUN_ID": "scheduling",
        "STEP_RECORD": str(record),
        "BUILD_STATUS": str(build_status),
        "NATIVE_STATUS": str(native_status),
    }
    result = subprocess.run(
        ["bash", str(tools / "run_api_tests.sh"), "--tier", "nightly"],
        env=env,
        capture_output=True,
        text=True,
        timeout=30,
    )
    assert (result.returncode == 0) is success
    assert [
        json.loads(line) for line in record.read_text().splitlines()
    ] == expected_steps
    evidence = tmp_path / "docs/gates/logs/scheduling"
    summary = evidence / "api-nightly-summary.json"
    assert summary.exists() is success
    if success:
        assert json.loads(summary.read_text())["status"] == "pass"
    if not build_status:
        assert (evidence / "api-nightly.xml").is_file()
        assert (evidence / "api-package-nightly.xml").is_file()
