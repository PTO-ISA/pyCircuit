"""Independent subprocess/receipt safety tests for the shared example verifier."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import subprocess
import sys
from pathlib import Path
from types import ModuleType

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[1]
VERIFIER = ROOT / "cmake/verify_example.py"


def executable(path: Path, body: str) -> Path:
    path.write_text(f"#!{sys.executable}\n" + body)
    path.chmod(0o755)
    return path


@pytest.fixture
def helper() -> ModuleType:
    spec = importlib.util.spec_from_file_location(
        "example_verifier_under_test", VERIFIER
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture
def case(tmp_path: Path) -> dict[str, Path]:
    source, build, include = (
        tmp_path / name for name in ("source", "build", "include")
    )
    source.mkdir()
    build.mkdir()
    (include / "verilog").mkdir(parents=True)
    (source / "model.py").write_text("# candidate hardware source\n")
    (source / "config.json").write_text('{"finite": true}\n')
    (source / "rtl_tb.sv").write_text("module tb; endmodule\n")
    (include / "verilog" / "primitive.v").write_text("module primitive; endmodule\n")
    (build / "model.ac").write_text("candidate hardware artifact\n")
    for target, name, role in (
        ("cpp", "model.cpp", "implementation"),
        ("verilog", "design_top.sv", "rtl"),
    ):
        directory = build / target
        directory.mkdir()
        (directory / name).write_text("generated candidate payload\n")
        (directory / "generated.json").write_text(
            json.dumps(
                {
                    "files": [{"path": name, "role": role}],
                }
            )
            + "\n"
        )
    runner = executable(tmp_path / "runner", "print('WORK one')\nprint('WORK two')\n")
    # The controlled external builder still launches a real second executable,
    # keeping process timeout/log/trace coordination under test, not HDL lowering.
    rtl_program = f"#!{sys.executable}\nprint('WORK one')\nprint('WORK two')\n"
    verilator = executable(
        tmp_path / "rtl-builder",
        f"""import sys
from pathlib import Path
output = Path(sys.argv[sys.argv.index('--Mdir') + 1]) / 'Vpycircuit_example'
output.write_text({rtl_program!r})
output.chmod(0o755)
""",
    )
    return {
        "source": source,
        "build": build,
        "include": include,
        "runner": runner,
        "verilator": verilator,
    }


def verify(helper: ModuleType, case: dict[str, Path], **kwargs: float) -> None:
    helper.verify(
        case["source"],
        case["build"],
        case["runner"],
        case["include"],
        case["verilator"],
        **kwargs,
    )


def stale_receipt(case: dict[str, Path]) -> Path:
    path = case["build"] / "verification.json"
    path.write_text('{"stale_success": true}\n')
    return path


def records(case: dict[str, Path]) -> list[dict]:
    return json.loads((case["build"] / "execution.json").read_text())


@pytest.mark.parametrize("timeout", [None, 0.75])
def test_success_preserves_trace_contract_and_uses_configured_timeout(
    helper: ModuleType,
    case: dict[str, Path],
    monkeypatch: pytest.MonkeyPatch,
    timeout: float | None,
) -> None:
    receipt = stale_receipt(case)
    real_run = subprocess.run
    observed = []

    def recording_run(*args, **kwargs):
        observed.append(kwargs["timeout"])
        return real_run(*args, **kwargs)

    monkeypatch.setattr(helper.subprocess, "run", recording_run)
    verify(helper, case, **({} if timeout is None else {"timeout": timeout}))

    expected_timeout = 180 if timeout is None else timeout
    assert observed == [expected_timeout] * 4
    result = json.loads(receipt.read_text())
    assert result["schema"] == "pycircuit-example-verification-v1"
    assert result["workers"] == [1, 2] and result["rtl"] == "verilator"
    assert result["work_samples"] == 2 and "stale_success" not in result
    assert "source/model.py" in result["inputs"]
    assert "build/cpp/model.cpp" in result["inputs"]
    assert "build/verilog/design_top.sv" in result["inputs"]
    assert result["execution_sha256"] == helper.digest(case["build"] / "execution.json")
    for record in records(case):
        assert record["exit_status"] == 0 and record["timed_out"] is False
        assert record["timeout_seconds"] == expected_timeout
        assert record["elapsed_seconds"] >= 0


@pytest.mark.parametrize("timeout", ["0", "-1", "nan", "inf", "-inf"])
def test_cli_rejects_invalid_timeout_before_launch(
    case: dict[str, Path],
    timeout: str,
) -> None:
    command = [sys.executable, str(VERIFIER)]
    for name, path in case.items():
        command.extend(["--" + name, str(path)])
    command.append("--timeout=" + timeout)
    result = subprocess.run(
        command, capture_output=True, text=True, check=False, timeout=5
    )
    assert result.returncode == 2
    assert "timeout" in result.stderr and "positive" in result.stderr
    assert not (case["build"] / "execution.json").exists()
    assert not (case["build"] / "verification.json").exists()


@pytest.mark.parametrize("timeout", [0, -1, float("nan"), float("inf")])
def test_direct_invalid_timeout_removes_stale_success(
    helper: ModuleType,
    case: dict[str, Path],
    timeout: float,
) -> None:
    receipt = stale_receipt(case)
    with pytest.raises(ValueError, match="positive"):
        verify(helper, case, timeout=timeout)
    assert not receipt.exists()
    assert not (case["build"] / "execution.json").exists()


def test_nonzero_child_preserves_logs_and_removes_prior_success(
    helper: ModuleType,
    case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    executable(
        case["runner"],
        "import sys\nprint('partial native output', flush=True)\n"
        "print('child rejected input', file=sys.stderr, flush=True)\nsys.exit(7)\n",
    )
    with pytest.raises(RuntimeError, match="serial: 7"):
        verify(helper, case, timeout=2)
    assert not receipt.exists()
    assert (case["build"] / "serial.stdout").read_text() == "partial native output\n"
    assert (case["build"] / "serial.stderr").read_text() == "child rejected input\n"
    (record,) = records(case)
    assert record["exit_status"] == 7 and record["timed_out"] is False
    assert record["timeout_seconds"] == 2 and record["elapsed_seconds"] >= 0


def test_real_timeout_preserves_flushed_output_and_removes_prior_success(
    helper: ModuleType,
    case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    executable(
        case["runner"],
        "import sys, time\nprint('started native', flush=True)\n"
        "print('waiting native', file=sys.stderr, flush=True)\ntime.sleep(5)\n",
    )
    # Allow interpreter startup under concurrent native builds, while keeping
    # the timeout well below the child's five-second sleep.
    with pytest.raises(RuntimeError, match="serial.*timed out"):
        verify(helper, case, timeout=2)
    assert not receipt.exists()
    assert (case["build"] / "serial.stdout").read_text() == "started native\n"
    assert (case["build"] / "serial.stderr").read_text() == "waiting native\n"
    (record,) = records(case)
    assert record["exit_status"] == 124 and record["timed_out"] is True
    assert record["timeout_seconds"] == 2 and record["elapsed_seconds"] >= 2


@pytest.mark.parametrize("as_bytes", [False, True])
def test_timeout_supports_both_subprocess_partial_stream_types(
    helper: ModuleType,
    case: dict[str, Path],
    monkeypatch: pytest.MonkeyPatch,
    as_bytes: bool,
) -> None:
    receipt = stale_receipt(case)
    stdout, stderr = "partial output\n", "partial error\n"

    def timed_out(command, **kwargs):
        raise subprocess.TimeoutExpired(
            command,
            kwargs["timeout"],
            output=stdout.encode() if as_bytes else stdout,
            stderr=stderr.encode() if as_bytes else stderr,
        )

    monkeypatch.setattr(helper.subprocess, "run", timed_out)
    with pytest.raises(RuntimeError, match="timed out"):
        verify(helper, case, timeout=0.25)
    assert not receipt.exists()
    assert (case["build"] / "serial.stdout").read_text() == stdout
    assert (case["build"] / "serial.stderr").read_text() == stderr
    (record,) = records(case)
    assert record["exit_status"] == 124 and record["timed_out"] is True
    assert record["timeout_seconds"] == 0.25


def test_changed_candidate_cannot_publish_success(
    helper: ModuleType,
    case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    builder = case["verilator"]
    builder.write_text(
        builder.read_text()
        + f"Path({str(case['source'] / 'model.py')!r}).write_text('changed candidate\\n')\n"
    )
    with pytest.raises(AssertionError, match="inputs changed"):
        verify(helper, case)
    assert not receipt.exists()
    assert len(records(case)) == 4 and all(
        row["exit_status"] == 0 for row in records(case)
    )


def test_missing_generated_payload_removes_stale_success_before_any_run(
    helper: ModuleType,
    case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    (case["build"] / "cpp" / "model.cpp").unlink()
    with pytest.raises(FileNotFoundError):
        verify(helper, case)
    assert not receipt.exists()
    assert not (case["build"] / "execution.json").exists()


@pytest.mark.parametrize("record", ["FRAME", "FAILURE", "CASES"])
def test_native_check_disagreement_cannot_publish_success(
    helper: ModuleType,
    case: dict[str, Path],
    record: str,
) -> None:
    receipt = stale_receipt(case)
    executable(
        case["runner"],
        "import sys\nprint('WORK one')\nprint('WORK two')\n"
        f"print({record!r}, sys.argv[-1])\n",
    )
    with pytest.raises(AssertionError, match="serial/parallel"):
        verify(helper, case)
    assert not receipt.exists()
    assert len(records(case)) == 2


@pytest.fixture
def system_case(case: dict[str, Path], tmp_path: Path) -> dict[str, Path]:
    """Synthetic receipts test verifier orchestration, never hardware semantics."""
    toolchain = tmp_path / "toolchain"
    (toolchain / "bin").mkdir(parents=True)
    for name in (
        "pycircuit",
        "pycircuit-source-unit",
        "pycircuit-link",
        "pycircuit-emit",
    ):
        (toolchain / "bin" / name).write_text("candidate native helper\n")
    (toolchain / "lib").mkdir()
    runtime = toolchain / "lib/libpyc6_runtime.so"
    runtime.write_text("candidate native runtime\n")
    for target in ("cpp", "verilog"):
        directory = case["build"] / target
        (directory / "system.ac").write_text("identical final common hardware IR\n")
        units = directory / "units" / "model"
        units.mkdir(parents=True)
        (units / "model.ac").write_text("candidate source import\n")
        bundle = directory / target
        bundle.mkdir()
        (bundle / "model.payload").write_text("candidate emitted payload\n")
        identity = {"entry": "sample.model.Bench", "entry_source": "sample/model"}
        metadata = bundle / "simulation_verification.json"
        metadata.write_text(json.dumps({**identity, "source_checks": 1}) + "\n")
        (bundle / "generated.json").write_text(
            json.dumps(
                {
                    **identity,
                    "files": [
                        {"path": "model.payload", "role": "implementation"},
                        {"path": "simulation_verification.json", "role": "metadata"},
                    ],
                }
            )
            + "\n"
        )
        binary = (
            directory
            / "simulation"
            / target
            / "bin"
            / ("pycircuit_sim.exe" if sys.platform == "win32" else "pycircuit_sim")
        )
        binary.parent.mkdir(parents=True)
        binary.write_text("candidate compiled simulation executable\n")
        bound = {
            str(path): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in (
                binary,
                runtime,
                bundle / "generated.json",
                metadata,
                bundle / "model.payload",
            )
        }
        (directory / "run-execution.json").write_text(
            json.dumps(
                {
                    "status": "success",
                    "command": [str(binary)],
                    "inputs": bound,
                    "inputs_after": bound,
                }
            )
            + "\n"
        )
    return {**case, "toolchain": toolchain, "driver": toolchain / "bin/pycircuit"}


def refresh_execution_inputs(case, target):
    path = case["build"] / target / "run-execution.json"
    row = json.loads(path.read_text())
    row["inputs"] = {
        name: hashlib.sha256(Path(name).read_bytes()).hexdigest()
        for name in row["inputs"]
    }
    row["inputs_after"] = row["inputs"]
    path.write_text(json.dumps(row) + "\n")


def system_records(*, observations: bool = True) -> list[dict]:
    trace = (
        [
            {"kind": "report", "evaluation_epoch": epoch, "value": str(epoch)}
            for epoch in (0, 3)
        ]
        if observations
        else []
    )
    return [*trace, {"kind": "result", "status": "TERMINATED", "epoch_time": 4}]


def system_runs(helper, monkeypatch, outputs, on_run=None):
    calls = []

    def run(command, **kwargs):
        index = len(calls)
        calls.append((command, kwargs))
        if on_run:
            on_run(index)
        return subprocess.CompletedProcess(
            command, 0, "\n".join(json.dumps(row) for row in outputs[index]) + "\n", ""
        )

    monkeypatch.setattr(helper.subprocess, "run", run)
    return calls


def verify_system(helper, case, **kwargs):
    helper.verify_system(
        case["source"],
        case["build"],
        case["driver"],
        case["include"],
        case["toolchain"],
        kwargs.get("cycles", 2),
        kwargs.get("timeout", 1.25),
    )


def test_system_full_duration_trace_and_input_receipt(helper, system_case, monkeypatch):
    calls = system_runs(helper, monkeypatch, [system_records()] * 3)
    verify_system(helper, system_case)
    receipt = json.loads((system_case["build"] / "verification.json").read_text())
    assert receipt["schema"] == "pycircuit-system-example-verification-v1"
    assert receipt["cycles"] == 2 and receipt["sampling_epochs"] == 4
    assert receipt["observations"] == 2 and receipt["source_checks"] == 1
    assert receipt["workers"] == [1, 2] and receipt["rtl"] == "verilator"
    executed_inputs = {}
    for target in ("cpp", "verilog"):
        executed_inputs.update(
            json.loads(
                (system_case["build"] / target / "run-execution.json").read_text()
            )["inputs"]
        )
    assert receipt["execution_inputs"] == executed_inputs
    assert receipt["execution_sha256"] == helper.digest(
        system_case["build"] / "execution.json"
    )
    assert (
        receipt["trace_sha256"]
        == helper.hashlib.sha256(
            json.dumps(system_records()[:-1], sort_keys=True).encode()
        ).hexdigest()
    )
    for (command, kwargs), (target, workers) in zip(
        calls, (("cpp", "1"), ("cpp", "2"), ("verilog", "1")), strict=True
    ):
        assert command == [
            str(system_case["driver"]),
            "run",
            str(system_case["source"]),
            "--target",
            target,
            "--build-dir",
            str(system_case["build"] / target),
            "--toolchain",
            str(system_case["toolchain"]),
            "--cycles",
            "2",
            "--workers",
            workers,
            "--timeout",
            "2",
        ]
        assert kwargs["timeout"] == 1.25
    for label, path in {
        "source/model.py": system_case["source"] / "model.py",
        "cpp/system.ac": system_case["build"] / "cpp/system.ac",
        "cpp/units/model/model.ac": system_case["build"] / "cpp/units/model/model.ac",
        "cpp/cpp/model.payload": system_case["build"] / "cpp/cpp/model.payload",
        "verilog/verilog/generated.json": system_case["build"]
        / "verilog/verilog/generated.json",
        "runtime-rtl/primitive.v": system_case["include"] / "verilog/primitive.v",
        "toolchain/pycircuit-emit": system_case["toolchain"] / "bin/pycircuit-emit",
    }.items():
        assert receipt["inputs"][label] == helper.digest(path)


def test_assert_only_system_accepts_selected_root_compiler_metadata(
    helper, system_case, monkeypatch
):
    system_runs(helper, monkeypatch, [system_records(observations=False)] * 3)
    verify_system(helper, system_case)
    receipt = json.loads((system_case["build"] / "verification.json").read_text())
    assert receipt["observations"] == 0 and receipt["source_checks"] == 1


@pytest.mark.parametrize(
    "final_ir",
    [
        "module {}\n",
        'module { "ac.expect"() : () -> () } // unreachable checked root\n',
    ],
)
def test_unchecked_root_refuses_empty_or_unreachable_expect_closure(
    helper, system_case, monkeypatch, final_ir
):
    proof = stale_receipt(system_case)
    # An unrelated retained assertion is not an obligation of the selected root.
    for target in ("cpp", "verilog"):
        (system_case["build"] / target / "system.ac").write_text(final_ir)
        metadata = (
            system_case["build"] / target / target / "simulation_verification.json"
        )
        row = json.loads(metadata.read_text())
        row["source_checks"] = 0
        metadata.write_text(json.dumps(row) + "\n")
        refresh_execution_inputs(system_case, target)
    system_runs(helper, monkeypatch, [system_records(observations=False)] * 3)
    with pytest.raises(ValueError, match="neither source checks nor observations"):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize(
    "bad_result",
    [
        [],
        [{"kind": "result", "status": "TERMINATED", "epoch_time": 4}] * 2,
        [{"kind": "result", "status": "FAILED", "epoch_time": 4}],
        *[
            [{"kind": "result", "status": "TERMINATED", "epoch_time": epoch}]
            for epoch in (0, 3, 5, 4.5)
        ],
    ],
)
def test_system_requires_one_success_result_at_exact_duration(
    helper, system_case, monkeypatch, bad_result
):
    proof = stale_receipt(system_case)
    system_runs(helper, monkeypatch, [system_records()[:-1] + bad_result] * 3)
    with pytest.raises(ValueError, match="full sampling duration"):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize("epoch", [-1, 4, 100, 1.5])
def test_system_refuses_observations_outside_complete_history(
    helper, system_case, monkeypatch, epoch
):
    proof = stale_receipt(system_case)
    rows = system_records()
    rows[0]["evaluation_epoch"] = epoch
    system_runs(helper, monkeypatch, [rows] * 3)
    with pytest.raises(ValueError, match="outside its sampling history"):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize("changed_run", [1, 2])
def test_system_requires_identical_complete_trace_across_workers_and_targets(
    helper, system_case, monkeypatch, changed_run
):
    proof = stale_receipt(system_case)
    outputs = [system_records() for _ in range(3)]
    outputs[changed_run][0]["value"] = "wrong"
    system_runs(helper, monkeypatch, outputs)
    with pytest.raises(ValueError, match="observations disagree"):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize(
    "component", ["source", "runtime", "toolchain", "cpp_payload", "cpp_unit"]
)
@pytest.mark.parametrize("mutation_run", [1, 2])
def test_system_changed_candidate_cannot_publish_success(
    helper, system_case, monkeypatch, component, mutation_run
):
    proof = stale_receipt(system_case)
    path = {
        "source": system_case["source"] / "model.py",
        "runtime": system_case["include"] / "verilog/primitive.v",
        "toolchain": system_case["toolchain"] / "bin/pycircuit-emit",
        "cpp_payload": system_case["build"] / "cpp/cpp/model.payload",
        "cpp_unit": system_case["build"] / "cpp/units/model/model.ac",
    }[component]

    def mutate(index):
        # Mutation after serial input binding must invalidate all target evidence.
        if index == mutation_run:
            path.write_text("candidate changed during verification\n")

    system_runs(helper, monkeypatch, [system_records()] * 3, on_run=mutate)
    with pytest.raises((ValueError, AssertionError), match="changed"):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize(
    "setting, value",
    [("cycles", 0), ("cycles", -1), ("timeout", 0), ("timeout", float("nan"))],
)
def test_system_invalid_limit_removes_stale_success_before_launch(
    helper, system_case, monkeypatch, setting, value
):
    proof = stale_receipt(system_case)
    calls = system_runs(helper, monkeypatch, [])
    with pytest.raises(ValueError, match="positive"):
        verify_system(helper, system_case, **{setting: value})
    assert not proof.exists()
    assert calls == []


@pytest.mark.parametrize("as_bytes", [False, True])
def test_system_timeout_preserves_partial_streams_and_refuses_success(
    helper, system_case, monkeypatch, as_bytes
):
    proof = stale_receipt(system_case)

    def timed_out(command, **kwargs):
        raise subprocess.TimeoutExpired(
            command,
            kwargs["timeout"],
            output=b"partial output\n" if as_bytes else "partial output\n",
            stderr=b"partial error\n" if as_bytes else "partial error\n",
        )

    monkeypatch.setattr(helper.subprocess, "run", timed_out)
    with pytest.raises(RuntimeError, match="serial: 124"):
        verify_system(helper, system_case)
    assert not proof.exists()
    assert (system_case["build"] / "serial.stdout").read_text() == "partial output\n"
    assert (system_case["build"] / "serial.stderr").read_text() == "partial error\n"
    (record,) = records(system_case)
    assert record["exit_status"] == 124 and record["timeout_seconds"] == 1.25


@pytest.mark.parametrize(
    "field, value",
    [
        ("entry", "sample.foreign.Unchecked"),
        ("entry_source", "sample/foreign"),
        ("source_checks", -1),
        ("source_checks", 1.0),
        ("source_checks", True),
        ("source_checks", "1"),
        ("extra", "unregistered authority"),
    ],
)
def test_system_metadata_must_identify_selected_root_with_exact_nonnegative_count(
    helper, system_case, monkeypatch, field, value
):
    proof = stale_receipt(system_case)
    metadata = system_case["build"] / "cpp/cpp/simulation_verification.json"
    row = json.loads(metadata.read_text())
    row[field] = value
    metadata.write_text(json.dumps(row) + "\n")
    refresh_execution_inputs(system_case, "cpp")
    system_runs(helper, monkeypatch, [system_records()] * 3)
    with pytest.raises(ValueError, match="selected root"):
        verify_system(helper, system_case)
    assert not proof.exists()


def test_system_metadata_must_agree_across_targets(helper, system_case, monkeypatch):
    proof = stale_receipt(system_case)
    metadata = system_case["build"] / "verilog/verilog/simulation_verification.json"
    row = json.loads(metadata.read_text())
    row["source_checks"] = 2
    metadata.write_text(json.dumps(row) + "\n")
    refresh_execution_inputs(system_case, "verilog")
    system_runs(helper, monkeypatch, [system_records()] * 3)
    with pytest.raises(ValueError, match="metadata changed across"):
        verify_system(helper, system_case)
    assert not proof.exists()


def test_system_cpp_and_rtl_must_consume_identical_final_ir(
    helper, system_case, monkeypatch
):
    proof = stale_receipt(system_case)
    (system_case["build"] / "verilog/system.ac").write_text(
        "different final common hardware IR\n"
    )
    system_runs(helper, monkeypatch, [system_records()] * 3)
    with pytest.raises(ValueError, match="final IR changed across"):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize(
    "defect",
    ["failed", "wrong-binary", "missing-binary", "after-mismatch", "stale-hash"],
)
def test_system_requires_successful_receipt_bound_to_actual_simulation_binary(
    helper, system_case, monkeypatch, defect
):
    proof = stale_receipt(system_case)
    path = system_case["build"] / "cpp/run-execution.json"
    row = json.loads(path.read_text())
    binary = row["command"][0]
    if defect == "failed":
        row["status"] = "failed"
    elif defect == "wrong-binary":
        row["command"][0] = str(system_case["driver"])
    elif defect == "missing-binary":
        del row["inputs"][binary]
        del row["inputs_after"][binary]
    elif defect == "after-mismatch":
        row["inputs_after"][binary] = "f" * 64
    else:
        row["inputs"][binary] = row["inputs_after"][binary] = "f" * 64
    path.write_text(json.dumps(row) + "\n")
    system_runs(helper, monkeypatch, [system_records()] * 3)
    with pytest.raises(
        ValueError, match="bound binary receipt|execution inputs changed"
    ):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize("component", ["binary", "runtime"])
@pytest.mark.parametrize("mutation_run", [0, 1, 2])
def test_system_binary_and_native_runtime_mutation_cannot_publish_success(
    helper, system_case, monkeypatch, component, mutation_run
):
    proof = stale_receipt(system_case)
    row = json.loads((system_case["build"] / "cpp/run-execution.json").read_text())
    path = (
        Path(row["command"][0])
        if component == "binary"
        else system_case["toolchain"] / "lib/libpyc6_runtime.so"
    )

    def mutate(index):
        if index == mutation_run:
            path.write_text("changed execution candidate\n")

    system_runs(helper, monkeypatch, [system_records()] * 3, on_run=mutate)
    with pytest.raises(ValueError, match="changed"):
        verify_system(helper, system_case)
    assert not proof.exists()


@pytest.mark.parametrize("field", ["entry", "entry_source", "source_checks"])
def test_system_metadata_requires_all_selected_root_identity_fields(
    helper, system_case, monkeypatch, field
):
    proof = stale_receipt(system_case)
    metadata = system_case["build"] / "cpp/cpp/simulation_verification.json"
    row = json.loads(metadata.read_text())
    del row[field]
    metadata.write_text(json.dumps(row) + "\n")
    refresh_execution_inputs(system_case, "cpp")
    system_runs(helper, monkeypatch, [system_records()] * 3)
    with pytest.raises(ValueError, match="selected root"):
        verify_system(helper, system_case)
    assert not proof.exists()
