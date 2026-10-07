"""Independent subprocess/receipt safety tests for the shared example verifier."""

from __future__ import annotations

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
    spec = importlib.util.spec_from_file_location("example_verifier_under_test", VERIFIER)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture
def case(tmp_path: Path) -> dict[str, Path]:
    source, build, include = (tmp_path / name for name in ("source", "build", "include"))
    source.mkdir()
    build.mkdir()
    (include / "verilog").mkdir(parents=True)
    (source / "model.py").write_text("# candidate hardware source\n")
    (source / "config.json").write_text('{"finite": true}\n')
    (source / "rtl_tb.sv").write_text("module tb; endmodule\n")
    (include / "verilog" / "primitive.v").write_text("module primitive; endmodule\n")
    (build / "model.ac").write_text("candidate hardware artifact\n")
    for target, name, role in (("cpp", "model.cpp", "implementation"),
                               ("verilog", "design_top.sv", "rtl")):
        directory = build / target
        directory.mkdir()
        (directory / name).write_text("generated candidate payload\n")
        (directory / "generated.json").write_text(json.dumps({
            "files": [{"path": name, "role": role}],
        }) + "\n")
    runner = executable(tmp_path / "runner", "print('WORK one')\nprint('WORK two')\n")
    # The controlled external builder still launches a real second executable,
    # keeping process timeout/log/trace coordination under test, not HDL lowering.
    rtl_program = f"#!{sys.executable}\nprint('WORK one')\nprint('WORK two')\n"
    verilator = executable(tmp_path / "rtl-builder", f"""import sys
from pathlib import Path
output = Path(sys.argv[sys.argv.index('--Mdir') + 1]) / 'Vpycircuit_example'
output.write_text({rtl_program!r})
output.chmod(0o755)
""")
    return {"source": source, "build": build, "include": include,
            "runner": runner, "verilator": verilator}


def verify(helper: ModuleType, case: dict[str, Path], **kwargs: float) -> None:
    helper.verify(case["source"], case["build"], case["runner"],
                  case["include"], case["verilator"], **kwargs)


def stale_receipt(case: dict[str, Path]) -> Path:
    path = case["build"] / "verification.json"
    path.write_text('{"stale_success": true}\n')
    return path


def records(case: dict[str, Path]) -> list[dict]:
    return json.loads((case["build"] / "execution.json").read_text())


@pytest.mark.parametrize("timeout", [None, 0.75])
def test_success_preserves_trace_contract_and_uses_configured_timeout(
    helper: ModuleType, case: dict[str, Path], monkeypatch: pytest.MonkeyPatch,
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
    case: dict[str, Path], timeout: str,
) -> None:
    command = [sys.executable, str(VERIFIER)]
    for name, path in case.items():
        command.extend(["--" + name, str(path)])
    command.append("--timeout=" + timeout)
    result = subprocess.run(command, capture_output=True, text=True, check=False, timeout=5)
    assert result.returncode == 2
    assert "timeout" in result.stderr and "positive" in result.stderr
    assert not (case["build"] / "execution.json").exists()
    assert not (case["build"] / "verification.json").exists()


@pytest.mark.parametrize("timeout", [0, -1, float("nan"), float("inf")])
def test_direct_invalid_timeout_removes_stale_success(
    helper: ModuleType, case: dict[str, Path], timeout: float,
) -> None:
    receipt = stale_receipt(case)
    with pytest.raises(ValueError, match="positive"):
        verify(helper, case, timeout=timeout)
    assert not receipt.exists()
    assert not (case["build"] / "execution.json").exists()


def test_nonzero_child_preserves_logs_and_removes_prior_success(
    helper: ModuleType, case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    executable(case["runner"], "import sys\nprint('partial native output', flush=True)\n"
               "print('child rejected input', file=sys.stderr, flush=True)\nsys.exit(7)\n")
    with pytest.raises(RuntimeError, match="serial: 7"):
        verify(helper, case, timeout=2)
    assert not receipt.exists()
    assert (case["build"] / "serial.stdout").read_text() == "partial native output\n"
    assert (case["build"] / "serial.stderr").read_text() == "child rejected input\n"
    record, = records(case)
    assert record["exit_status"] == 7 and record["timed_out"] is False
    assert record["timeout_seconds"] == 2 and record["elapsed_seconds"] >= 0


def test_real_timeout_preserves_flushed_output_and_removes_prior_success(
    helper: ModuleType, case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    executable(case["runner"], "import sys, time\nprint('started native', flush=True)\n"
               "print('waiting native', file=sys.stderr, flush=True)\ntime.sleep(5)\n")
    # Allow interpreter startup under concurrent native builds, while keeping
    # the timeout well below the child's five-second sleep.
    with pytest.raises(RuntimeError, match="serial.*timed out"):
        verify(helper, case, timeout=2)
    assert not receipt.exists()
    assert (case["build"] / "serial.stdout").read_text() == "started native\n"
    assert (case["build"] / "serial.stderr").read_text() == "waiting native\n"
    record, = records(case)
    assert record["exit_status"] == 124 and record["timed_out"] is True
    assert record["timeout_seconds"] == 2 and record["elapsed_seconds"] >= 2


@pytest.mark.parametrize("as_bytes", [False, True])
def test_timeout_supports_both_subprocess_partial_stream_types(
    helper: ModuleType, case: dict[str, Path], monkeypatch: pytest.MonkeyPatch,
    as_bytes: bool,
) -> None:
    receipt = stale_receipt(case)
    stdout, stderr = "partial output\n", "partial error\n"

    def timed_out(command, **kwargs):
        raise subprocess.TimeoutExpired(command, kwargs["timeout"],
                                        output=stdout.encode() if as_bytes else stdout,
                                        stderr=stderr.encode() if as_bytes else stderr)

    monkeypatch.setattr(helper.subprocess, "run", timed_out)
    with pytest.raises(RuntimeError, match="timed out"):
        verify(helper, case, timeout=0.25)
    assert not receipt.exists()
    assert (case["build"] / "serial.stdout").read_text() == stdout
    assert (case["build"] / "serial.stderr").read_text() == stderr
    record, = records(case)
    assert record["exit_status"] == 124 and record["timed_out"] is True
    assert record["timeout_seconds"] == 0.25


def test_changed_candidate_cannot_publish_success(
    helper: ModuleType, case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    builder = case["verilator"]
    builder.write_text(builder.read_text() +
                       f"Path({str(case['source'] / 'model.py')!r}).write_text('changed candidate\\n')\n")
    with pytest.raises(AssertionError, match="inputs changed"):
        verify(helper, case)
    assert not receipt.exists()
    assert len(records(case)) == 4 and all(row["exit_status"] == 0 for row in records(case))


def test_missing_generated_payload_removes_stale_success_before_any_run(
    helper: ModuleType, case: dict[str, Path],
) -> None:
    receipt = stale_receipt(case)
    (case["build"] / "cpp" / "model.cpp").unlink()
    with pytest.raises(FileNotFoundError):
        verify(helper, case)
    assert not receipt.exists()
    assert not (case["build"] / "execution.json").exists()


@pytest.mark.parametrize("record", ["FRAME", "FAILURE", "CASES"])
def test_native_check_disagreement_cannot_publish_success(
    helper: ModuleType, case: dict[str, Path], record: str,
) -> None:
    receipt = stale_receipt(case)
    executable(case["runner"], "import sys\nprint('WORK one')\nprint('WORK two')\n"
               f"print({record!r}, sys.argv[-1])\n")
    with pytest.raises(AssertionError, match="serial/parallel"):
        verify(helper, case)
    assert not receipt.exists()
    assert len(records(case)) == 2
