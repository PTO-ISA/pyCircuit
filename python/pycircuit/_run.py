"""Build and run a source system through the existing source-unit CMake flow."""

from __future__ import annotations

import hashlib
import json
import os
import signal
import subprocess
import time
from pathlib import Path

from ._driver import _DriverError
from ._generated_bundle import _execution_metadata
from ._publication import _PublicationError
from .packaged_toolchain import _installed_prefix_root, bundled_toolchain_root


def _invoke(
    command: list[str], *, timeout: int, env: dict[str, str], stdout=None
) -> None:
    output = {} if stdout is None else {"stdout": stdout}
    if os.name == "posix":
        # CMake and build tools spawn compilers. Own a separate process group so
        # a timed-out build cannot leave those children consuming resources.
        with subprocess.Popen(
            command, env=env, start_new_session=True, **output
        ) as result:
            try:
                result.wait(timeout=timeout)
            except (subprocess.TimeoutExpired, KeyboardInterrupt):
                try:
                    os.killpg(result.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                result.wait()
                raise
    else:
        result = subprocess.run(
            command, env=env, timeout=timeout, check=False, **output
        )
    if result.returncode:
        raise _DriverError(f"command failed ({result.returncode}): {' '.join(command)}")


def run_command(
    *,
    source: str,
    target: str,
    build_dir: str | None,
    toolchain: str | None,
    cycles: int | None,
    workers: int,
    timeout: int,
    execution: str | None = None,
    model: str | None = None,
    inputs: str | None = None,
    consumer_start: int = 0,
    memory: str | None = None,
) -> None:
    if execution not in (None, "functional", "timing"):
        raise _DriverError("execution must be functional or timing")
    if memory is not None and (
        inputs is not None
        or model is None
        or execution is None
        or cycles is not None
        or workers != 1
    ):
        raise _DriverError(
            "memory run requires execution/model, one worker and config-owned cycles; do not combine inputs/cycles"
        )
    if inputs is not None and (execution is None or model is None):
        raise _DriverError("model inputs require --execution and --model")
    if (
        memory is None
        and inputs is None
        and (model is not None or consumer_start or execution == "functional")
    ):
        raise _DriverError("functional/model run requires --model and --inputs")
    if consumer_start < 0 or (execution == "functional" and consumer_start):
        raise _DriverError("consumer-start is a nonnegative timing-only condition")
    if model is not None and not all(part.isidentifier() for part in model.split(".")):
        raise _DriverError("model requires a qualified source declaration name")
    request_rows = None
    if inputs is not None:
        from ._model_run import read_requests

        request_rows = read_requests(Path(inputs))
        if execution == "functional" and cycles is not None:
            raise _DriverError(
                "functional input streams define logical steps; do not supply --cycles"
            )
    if cycles is not None and (cycles <= 0 or cycles > (2**63 - 1) // 2):
        raise _DriverError("cycles must be positive and fit the finite runtime limit")
    if workers <= 0 or timeout <= 0:
        raise _DriverError("workers and timeout must be positive")
    if target == "verilog" and workers != 1:
        raise _DriverError(
            "workers selects native simulation threads; use one for Verilog"
        )
    directory = Path(source).resolve()
    if not (directory / "CMakeLists.txt").is_file():
        raise _DriverError(
            "run expects an independent example directory with CMakeLists.txt"
        )
    prefix = toolchain or os.environ.get("PYC_TOOLCHAIN_ROOT")
    installed = (
        Path(prefix).resolve()
        if prefix
        else (_installed_prefix_root() or bundled_toolchain_root())
    )
    if installed is None:
        raise _DriverError(
            "an installed toolchain is required; pass --toolchain or set PYC_TOOLCHAIN_ROOT"
        )
    package = installed / "share/pycircuit/cmake"
    if not (package / "pycircuitConfig.cmake").is_file():
        raise _DriverError(f"toolchain has no Runtime CMake package: {package}")
    default_run = Path.cwd() / ".pycircuit_out/run" / directory.name
    if execution:
        default_run = default_run / execution / (model or "system")
    build = Path(build_dir).resolve() if build_dir else (default_run / target).resolve()
    if build == directory or build in directory.parents or directory in build.parents:
        raise _DriverError("run requires a build directory outside the example sources")
    cache = build / "CMakeCache.txt"
    if cache.is_file():
        owners = [
            line.partition("=")[2]
            for line in cache.read_text().splitlines()
            if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL=")
        ]
        if owners != [str(directory)]:
            raise _DriverError("build directory belongs to a different CMake source")
        selections = [
            line.partition("=")[2]
            for line in cache.read_text().splitlines()
            if line.startswith("PYC_EXAMPLE_RUN_TARGET:")
        ]
        if selections != [target]:
            raise _DriverError(
                "build directory is not an isolated run for this backend; "
                "choose a separate build directory"
            )
        for key, selected in (
            ("PYC_EXAMPLE_EXECUTION", execution),
            ("PYC_EXAMPLE_MODEL_TOP", model),
        ):
            found = [
                line.partition("=")[2]
                for line in cache.read_text().splitlines()
                if line.startswith(key + ":")
            ]
            if found != [selected] if selected is not None else found not in ([], [""]):
                raise _DriverError(
                    "build directory belongs to another execution/model selection"
                )
    execution_receipt = build / "run-execution.json"
    execution_receipt.unlink(missing_ok=True)
    if memory is not None:
        latest = build / "clocked-run/execution.json"
        latest.parent.mkdir(parents=True, exist_ok=True)
        latest.write_text(
            json.dumps(
                {
                    "schema": "pycircuit-clocked-memory-run-v1",
                    "status": "incomplete",
                    "events": None,
                    "stores": None,
                    "executed_ticks": None,
                    "model_cycles": None,
                    "validation": "new attempt; no completed execution claim",
                }
            )
            + "\n"
        )
    env = dict(os.environ, PYC_TOOLCHAIN_ROOT=str(installed))
    # Resolve all subprocesses through the selected installed compiler, not an
    # unrelated developer override inherited from an interactive shell.
    for name in (
        "PYCIRCUIT_SOURCE_COMPILER",
        "PYCIRCUIT_LINKER",
        "PYCIRCUIT_EMITTER",
        "PYCIRCUIT_NATIVE_BUILD",
    ):
        env.pop(name, None)
    configure = [
        "cmake",
        "-S",
        str(directory),
        "-B",
        str(build),
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        f"-Dpycircuit_DIR={package}",
        "-DPYC_EXAMPLE_VERIFY=OFF",
        f"-DPYC_EXAMPLE_RUN_TARGET={target}",
    ]
    if execution is not None:
        configure.append(f"-DPYC_EXAMPLE_EXECUTION:STRING={execution}")
    if model is not None:
        configure.append(f"-DPYC_EXAMPLE_MODEL_TOP:STRING={model}")
    if memory is not None and target == "verilog":
        configure.append("-DPYC_EXAMPLE_RTL_MODE:STRING=physical")
    configured_at = time.perf_counter()
    _invoke(
        configure,
        timeout=timeout,
        env=env,
    )
    configure_seconds = time.perf_counter() - configured_at
    compiled_at = time.perf_counter()
    _invoke(
        ["cmake", "--build", str(build), "--target", "pycircuit_simulation_artifacts"],
        timeout=timeout,
        env=env,
    )
    source_build_seconds = time.perf_counter() - compiled_at
    generated = build / target
    receipt = json.loads((generated / "generated.json").read_text())
    if memory is not None:
        from ._generated_bundle import _load_generated_bundle
        from ._memory_run import run_memory
        from ._publication import _publication_owner_generated

        try:
            _load_generated_bundle(
                generated,
                owner=_publication_owner_generated(
                    package=receipt["entry_source"]["package"],
                    path=receipt["entry_source"]["path"],
                    definition=receipt["entry"]["definition"],
                    target=target,
                ),
            )
        except _PublicationError as error:
            raise _DriverError(str(error)) from error
        run_memory(generated, memory, build / "clocked-run", installed, timeout)
        return
    if request_rows is not None:
        from ._model_run import run_requests

        run_requests(
            generated=generated,
            build=build,
            installed=installed,
            receipt=receipt,
            rows=request_rows,
            strategy=execution,
            target=target,
            cycles=cycles,
            consumer_start=consumer_start,
            workers=workers,
            timeout=timeout,
            env=env,
            invoke=_invoke,
            build_metrics={
                "configure_seconds": configure_seconds,
                "source_compile_link_emit_seconds": source_build_seconds,
            },
        )
        return
    try:
        sample_rows, needs_four_state = _execution_metadata(receipt)
    except _PublicationError as error:
        raise _DriverError(str(error)) from error
    if sample_rows and cycles is not None:
        raise _DriverError("--cycles is not allowed with finite System samples")
    effective_cycles = 10 if cycles is None else cycles
    expected_epochs = len(sample_rows) if sample_rows else 2 * effective_cycles
    reset_semantics = receipt.get("reset_semantics", "strict")
    if reset_semantics not in ("strict", "if_one"):
        raise _DriverError("generated reset semantics are invalid")
    required = "simulation_main.cpp" if target == "cpp" else "simulation_top.sv"
    if required not in {row["path"] for row in receipt["files"]}:
        raise _DriverError(
            "run requires a source @system root; a module emits a reusable design"
        )
    binary_build = build / "simulation" / target
    _invoke(
        [
            "cmake",
            "-S",
            str(generated),
            "-B",
            str(binary_build),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-Dpycircuit_DIR={package}",
        ],
        timeout=timeout,
        env=env,
    )
    _invoke(
        ["cmake", "--build", str(binary_build), "--target", "pycircuit_sim"],
        timeout=timeout,
        env=env,
    )
    executable = (
        binary_build
        / "bin"
        / ("pycircuit_sim.exe" if os.name == "nt" else "pycircuit_sim")
    )
    arguments = ["--workers", str(workers)] if target == "cpp" else []
    if not sample_rows:
        if target == "cpp":
            arguments = ["--cycles", str(effective_cycles), *arguments]
        else:
            arguments = [f"+cycles={effective_cycles}"]
    simulator = {"backend": "cpp" if target == "cpp" else "verilator"}
    candidates = [executable]
    backend_receipt = Path(str(executable) + ".backend.json")
    if target == "verilog" and backend_receipt.is_file():
        simulator = json.loads(backend_receipt.read_text())
        expected_backend = "iverilog" if needs_four_state else "verilator"
        if (
            type(simulator) is not dict
            or set(simulator) != {"backend", "compiler", "runner"}
            or simulator["backend"] != expected_backend
            or type(simulator["compiler"]) is not str
            or (expected_backend == "iverilog" and type(simulator["runner"]) is not str)
            or (expected_backend == "verilator" and simulator["runner"] is not None)
        ):
            raise _DriverError("simulation backend receipt is inconsistent")
        candidates.extend([backend_receipt, Path(simulator["compiler"])])
        if simulator["runner"] is not None:
            candidates.append(Path(simulator["runner"]))
    elif target == "verilog" and "needs_four_state" in receipt:
        raise _DriverError("simulation build produced no backend receipt")
    execution_metadata = {
        "backend": simulator["backend"],
        "reset_semantics": reset_semantics,
        "sampling_epochs": expected_epochs,
        "executable": str(executable),
    }
    candidates.extend(sorted(installed.glob("lib*/*pyc6_runtime*")))
    candidates.append(generated / "generated.json")
    for item in receipt["files"]:
        relative = Path(item["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise _DriverError("generated receipt contains an unsafe path")
        candidates.append(generated / relative)
    metadata = installed / "share/pycircuit/toolchain-metadata.json"
    if metadata.is_file():
        candidates.append(metadata)

    def execution_inputs():
        return {
            str(path): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in candidates
        }

    before = execution_inputs()
    command = [str(executable), *arguments]
    if simulator["backend"] == "iverilog":
        command.insert(0, simulator["runner"])
    try:
        _invoke(command, timeout=timeout, env=env)
    except Exception as error:
        execution_receipt.write_text(
            json.dumps(
                {
                    **execution_metadata,
                    "command": command,
                    "status": "failed",
                    "error": str(error),
                    "inputs": before,
                },
                indent=2,
            )
            + "\n"
        )
        raise
    after = execution_inputs()
    if after != before:
        execution_receipt.write_text(
            json.dumps(
                {
                    **execution_metadata,
                    "command": command,
                    "status": "changed",
                    "inputs": before,
                    "inputs_after": after,
                },
                indent=2,
            )
            + "\n"
        )
        raise _DriverError("simulation execution inputs changed during the run")
    execution_receipt.write_text(
        json.dumps(
            {
                **execution_metadata,
                "command": command,
                "status": "success",
                "inputs": before,
                "inputs_after": after,
            },
            indent=2,
        )
        + "\n"
    )
