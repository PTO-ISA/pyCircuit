"""Run-driver refusal boundaries before any build or simulation side effect."""

import hashlib
import json
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from threading import Barrier, Lock

import pytest
from pycircuit import _run
from pycircuit._driver import _DriverError
from pycircuit.cli import main

pytestmark = pytest.mark.unit
MAX_CYCLES = (2**63 - 1) // 2


@pytest.mark.skipif(
    not sys.platform.startswith("linux"), reason="Linux descendant process-state check"
)
def test_timeout_stops_owned_build_children_and_preserves_unrelated_process(tmp_path):
    pid_file = tmp_path / "compiler.pid"
    script = (
        "import pathlib,subprocess,sys,time; "
        "child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(60)']); "
        f"pathlib.Path({str(pid_file)!r}).write_text(str(child.pid)); "
        "time.sleep(60)"
    )
    command = [sys.executable, "-c", script]
    unrelated = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
    try:
        with pytest.raises(subprocess.TimeoutExpired) as error:
            _run._invoke(command, timeout=1, env=dict(os.environ))
        assert error.value.cmd == command
        assert error.value.timeout == 1
        assert unrelated.poll() is None
        child_pid = int(pid_file.read_text())
        # A killed grandchild can remain a zombie until the OS reaps it. Both
        # disappearance and a zombie establish it cannot keep compiling.
        deadline = time.monotonic() + 2
        while time.monotonic() < deadline:
            try:
                state = Path(f"/proc/{child_pid}/stat").read_text().split(") ", 1)[1][0]
            except FileNotFoundError:
                break
            if state == "Z":
                break
            time.sleep(0.01)
        else:
            pytest.fail("owned compiler child remained running after timeout")
    finally:
        unrelated.kill()
        unrelated.wait(timeout=5)


def _arguments(source: Path, prefix: Path, build: Path) -> dict:
    return {
        "source": str(source),
        "target": "cpp",
        "build_dir": str(build),
        "toolchain": str(prefix),
        "cycles": 3,
        "workers": 1,
        "timeout": 10,
    }


@pytest.fixture
def project(tmp_path: Path, monkeypatch):
    source = tmp_path / "source with spaces"
    source.mkdir()
    (source / "CMakeLists.txt").write_text("project(Test)\n")
    prefix = tmp_path / "installed toolchain"
    package = prefix / "share/pycircuit/cmake"
    package.mkdir(parents=True)
    (package / "pycircuitConfig.cmake").write_text("# test package\n")
    calls = []
    monkeypatch.setattr(
        _run, "_invoke", lambda command, **kwargs: calls.append(command)
    )
    return source, prefix, tmp_path / "build", calls


@pytest.mark.parametrize(
    "field,value",
    [
        ("cycles", 0),
        ("cycles", -1),
        ("cycles", MAX_CYCLES + 1),
        ("cycles", 2**64),
        ("workers", 0),
        ("timeout", 0),
    ],
)
def test_invalid_runtime_limits_never_configure(project, field, value):
    source, prefix, build, calls = project
    arguments = _arguments(source, prefix, build)
    arguments[field] = value
    with pytest.raises(_DriverError, match="positive|finite"):
        _run.run_command(**arguments)
    assert calls == []
    assert not build.exists()


def test_parallel_rtl_request_is_rejected_before_build(project):
    source, prefix, build, calls = project
    arguments = _arguments(source, prefix, build)
    arguments.update(target="verilog", workers=2)
    with pytest.raises(_DriverError, match="one for Verilog"):
        _run.run_command(**arguments)
    assert calls == []


@pytest.mark.parametrize("location", ["source", "descendant", "ancestor", "symlink"])
def test_build_path_must_be_outside_sources(project, location, tmp_path):
    source, prefix, build, calls = project
    if location == "source":
        build = source
    elif location == "descendant":
        build = source / "output"
    elif location == "ancestor":
        build = source.parent
    else:
        build = tmp_path / "source alias"
        build.symlink_to(source, target_is_directory=True)
    before = (source / "CMakeLists.txt").read_bytes()
    with pytest.raises(_DriverError, match="outside the example sources"):
        _run.run_command(**_arguments(source, prefix, build))
    assert calls == []
    assert (source / "CMakeLists.txt").read_bytes() == before


def test_other_project_cache_is_preserved(project):
    source, prefix, build, calls = project
    build.mkdir()
    cache = build / "CMakeCache.txt"
    cache.write_text("CMAKE_HOME_DIRECTORY:INTERNAL=/some/other/project\n")
    before = cache.read_bytes()
    with pytest.raises(_DriverError, match="different CMake source"):
        _run.run_command(**_arguments(source, prefix, build))
    assert calls == []
    assert cache.read_bytes() == before


@pytest.mark.parametrize("selection", [None, "", "verilog"])
def test_same_source_incompatible_cache_is_preserved(project, selection):
    source, prefix, build, calls = project
    build.mkdir()
    cache = f"CMAKE_HOME_DIRECTORY:INTERNAL={source}\n"
    if selection is not None:
        cache += f"PYC_EXAMPLE_RUN_TARGET:STRING={selection}\n"
    (build / "CMakeCache.txt").write_text(cache)
    (build / "module-output.ac").write_text("existing module artifact\n")
    (build / "run-execution.json").write_text('{"status":"success"}\n')
    before = {p.name: p.read_bytes() for p in build.iterdir()}
    with pytest.raises(_DriverError, match="not an isolated run for this backend"):
        _run.run_command(**_arguments(source, prefix, build))
    assert calls == []
    assert {p.name: p.read_bytes() for p in build.iterdir()} == before


@pytest.mark.parametrize("stage", ["configure", "artifacts", "binary"])
def test_same_role_retry_failure_invalidates_previous_execution_success(
    project, monkeypatch, stage
):
    source, prefix, build, _calls = project
    build.mkdir()
    (build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={source}\n"
        "PYC_EXAMPLE_RUN_TARGET:STRING=cpp\n"
    )
    execution = build / "run-execution.json"
    execution.write_text('{"status":"success","stale":true}\n')

    def invoke(command, **_kwargs):
        if stage == "configure" and command[:2] == ["cmake", "-S"]:
            raise _DriverError("configuration failed")
        if command[:2] == ["cmake", "--build"]:
            if command[-1] == "pycircuit_simulation_artifacts":
                if stage == "artifacts":
                    raise _DriverError("artifact build failed")
                _write_generated_bundle(build, "cpp")
            elif command[-1] == "pycircuit_sim":
                raise _DriverError("binary build failed")

    monkeypatch.setattr(_run, "_invoke", invoke)
    with pytest.raises(_DriverError, match="failed"):
        _run.run_command(**_arguments(source, prefix, build))
    assert (
        not execution.exists()
        or json.loads(execution.read_text())["status"] != "success"
    )


def _write_generated_bundle(directory, target):
    generated = directory / target
    generated.mkdir(parents=True, exist_ok=True)
    harness = "simulation_main.cpp" if target == "cpp" else "simulation_top.sv"
    (generated / harness).write_text(f"generated {target} simulation harness\n")
    (generated / "generated.json").write_text(
        json.dumps({"files": [{"path": harness}]})
    )


def _write_simulation_binary(directory):
    binary = directory / "bin/pycircuit_sim"
    binary.parent.mkdir(parents=True, exist_ok=True)
    binary.write_bytes(b"test simulation binary\n")
    return binary


def test_parallel_backends_do_not_reconfigure_each_others_default_build(
    project, tmp_path, monkeypatch
):
    source, prefix, _build, _calls = project
    monkeypatch.chdir(tmp_path)
    configured = {}
    lock = Lock()
    both_configured = Barrier(2)
    invocations = []

    def invoke(command, **_kwargs):
        with lock:
            invocations.append(command)
        if command[:2] == ["cmake", "-S"]:
            requested = [
                argument.partition("=")[2]
                for argument in command
                if argument.startswith("-DPYC_EXAMPLE_RUN_TARGET=")
            ]
            if requested:
                directory = Path(command[command.index("-B") + 1])
                with lock:
                    configured[directory] = requested[0]
                # Both configure commands finish before either builds, exposing
                # a shared CMake cache whose backend selection was overwritten.
                both_configured.wait(timeout=10)
        elif (
            command[:2] == ["cmake", "--build"]
            and command[-1] == "pycircuit_simulation_artifacts"
        ):
            directory = Path(command[2])
            with lock:
                target = configured[directory]
            _write_generated_bundle(directory, target)
        elif command[:2] == ["cmake", "--build"] and command[-1] == "pycircuit_sim":
            _write_simulation_binary(Path(command[2]))

    monkeypatch.setattr(_run, "_invoke", invoke)

    def execute(target):
        arguments = _arguments(source, prefix, tmp_path / "unused")
        arguments.update(target=target, build_dir=None)
        _run.run_command(**arguments)

    with ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(execute, ("cpp", "verilog")))
    assert len(configured) == 2
    assert set(configured.values()) == {"cpp", "verilog"}
    executables = [
        command[0]
        for command in invocations
        if "pycircuit_sim" in Path(command[0]).name
    ]
    assert len(executables) == 2 and executables[0] != executables[1]
    for directory in configured:
        receipt = json.loads((directory / "run-execution.json").read_text())
        assert receipt["status"] == "success"
        assert receipt["inputs"] == receipt["inputs_after"]
        assert set(receipt["inputs"]) == {
            receipt["command"][0],
            str(directory / configured[directory] / "generated.json"),
            str(
                directory
                / configured[directory]
                / (
                    "simulation_main.cpp"
                    if configured[directory] == "cpp"
                    else "simulation_top.sv"
                )
            ),
        }
        for path, digest in receipt["inputs"].items():
            assert digest == hashlib.sha256(Path(path).read_bytes()).hexdigest()


@pytest.mark.parametrize("mutation", ["binary", "runtime", "failure", "none"])
def test_run_execution_receipt_binds_actual_inputs_and_invalidates_old_success(
    project, monkeypatch, mutation
):
    source, prefix, build, _calls = project
    build.mkdir()
    execution = build / "run-execution.json"
    execution.write_text('{"status":"success","stale":true}\n')
    runtime = prefix / "lib/libpyc6_runtime.a"
    runtime.parent.mkdir()
    runtime.write_bytes(b"runtime archive before run\n")
    executed = []

    def invoke(command, **_kwargs):
        if command[:2] == ["cmake", "--build"]:
            if command[-1] == "pycircuit_simulation_artifacts":
                _write_generated_bundle(build, "cpp")
            elif command[-1] == "pycircuit_sim":
                _write_simulation_binary(Path(command[2]))
        elif Path(command[0]).name == "pycircuit_sim":
            executed.append(command)
            assert not execution.exists()
            if mutation == "failure":
                raise _DriverError("simulated binary execution failed (7)")
            if mutation in {"binary", "runtime"}:
                selected = Path(command[0]) if mutation == "binary" else runtime
                selected.write_bytes(b"changed during binary execution\n")

    monkeypatch.setattr(_run, "_invoke", invoke)
    if mutation == "failure":
        with pytest.raises(_DriverError, match="execution failed"):
            _run.run_command(**_arguments(source, prefix, build))
    elif mutation in {"binary", "runtime"}:
        with pytest.raises(_DriverError, match="inputs changed during the run"):
            _run.run_command(**_arguments(source, prefix, build))
    else:
        _run.run_command(**_arguments(source, prefix, build))
    assert len(executed) == 1
    receipt = json.loads(execution.read_text())
    assert "stale" not in receipt
    assert receipt["command"] == executed[0]
    assert str(runtime) in receipt["inputs"]
    if mutation == "failure":
        assert receipt["status"] == "failed"
        assert "execution failed (7)" in receipt["error"]
    elif mutation in {"binary", "runtime"}:
        assert receipt["status"] == "changed"
        changed = {
            path
            for path, digest in receipt["inputs"].items()
            if digest != receipt["inputs_after"][path]
        }
        assert changed == {executed[0][0] if mutation == "binary" else str(runtime)}
    else:
        assert receipt["status"] == "success"
        assert receipt["inputs"] == receipt["inputs_after"]
        for path, digest in receipt["inputs"].items():
            assert digest == hashlib.sha256(Path(path).read_bytes()).hexdigest()


def test_module_only_bundle_is_not_executed(project):
    source, prefix, build, calls = project
    generated = build / "cpp"
    generated.mkdir(parents=True)
    (generated / "generated.json").write_text('{"files":[{"path":"dut.hpp"}]}')
    with pytest.raises(_DriverError, match="source @system root"):
        _run.run_command(**_arguments(source, prefix, build))
    assert len(calls) == 2
    assert not (build / "simulation").exists()


@pytest.mark.parametrize("cycles", [0, MAX_CYCLES + 1])
def test_cli_runtime_diagnostic_has_no_traceback(tmp_path, capsys, cycles):
    assert main(["run", str(tmp_path), "--cycles", str(cycles)]) == 1
    captured = capsys.readouterr()
    assert captured.out == ""
    assert captured.err.startswith("pycircuit run: ")
    assert "Traceback" not in captured.err


def test_cli_cycles_omission_and_duplicates_preserve_intent(monkeypatch, capsys):
    captured = []
    monkeypatch.setattr(_run, "run_command", lambda **kwargs: captured.append(kwargs))
    assert main(["run", "example"]) == 0
    assert captured[-1]["cycles"] is None
    with pytest.raises(SystemExit) as duplicate:
        main(["run", "example", "--cycles", "2", "--cycles", "3"])
    assert duplicate.value.code == 2
    assert "duplicate --cycles" in capsys.readouterr().err
    assert len(captured) == 1


@pytest.mark.parametrize("cycles", [1, 3])
def test_finite_samples_reject_every_explicit_cycle_count(project, cycles):
    source, prefix, build, calls = project
    _write_generated_bundle(build, "cpp")
    path = build / "cpp/generated.json"
    receipt = json.loads(path.read_text())
    receipt.update(system_samples=["00", "1x", "0z"], needs_four_state=True)
    path.write_text(json.dumps(receipt))
    with pytest.raises(_DriverError, match="--cycles.*finite System samples"):
        _run.run_command(**{**_arguments(source, prefix, build), "cycles": cycles})
    assert len(calls) == 2
    assert not (build / "simulation").exists()


@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_finite_samples_use_bound_actual_backend_without_cycle_override(
    project, target
):
    source, prefix, build, calls = project
    _write_generated_bundle(build, target)
    path = build / target / "generated.json"
    receipt = json.loads(path.read_text())
    receipt.update(
        system_samples=["00", "1x", "0z"],
        needs_four_state=True,
        reset_semantics="if_one",
    )
    path.write_text(json.dumps(receipt))
    binary = _write_simulation_binary(build / "simulation" / target)
    if target == "verilog":
        compiler, runner = prefix / "iverilog", prefix / "vvp"
        compiler.write_text("four-state compiler")
        runner.write_text("four-state interpreter")
        Path(str(binary) + ".backend.json").write_text(
            json.dumps(
                {
                    "backend": "iverilog",
                    "compiler": str(compiler),
                    "runner": str(runner),
                }
            )
        )
    _run.run_command(
        **{**_arguments(source, prefix, build), "target": target, "cycles": None}
    )
    executed = json.loads((build / "run-execution.json").read_text())
    assert executed["sampling_epochs"] == 3
    assert executed["reset_semantics"] == "if_one"
    assert "--cycles" not in calls[-1] and not any(
        "+cycles=" in arg for arg in calls[-1]
    )
    if target == "verilog":
        assert calls[-1] == [str(runner), str(binary)]
        assert executed["backend"] == "iverilog"
        assert str(runner) in executed["inputs"] and str(compiler) in executed["inputs"]
    else:
        assert calls[-1] == [str(binary), "--workers", "1"]
        assert executed["backend"] == "cpp"


def test_generated_system_cmake_requires_four_state_tools_only_when_needed():
    from pycircuit._emit import _cmake

    four_state = _cmake(
        [],
        [],
        target="verilog",
        standard_sources=[],
        simulation=True,
        needs_four_state=True,
    )
    assert "find_program(PYCIRCUIT_IVERILOG iverilog REQUIRED)" in four_state
    assert '--vvp "${PYCIRCUIT_VVP}"' in four_state
    assert "find_program(PYCIRCUIT_VERILATOR" not in four_state
    binary = _cmake([], [], target="verilog", standard_sources=[], simulation=True)
    assert "find_program(PYCIRCUIT_VERILATOR verilator REQUIRED)" in binary
