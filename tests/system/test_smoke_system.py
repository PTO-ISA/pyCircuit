from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import uuid
from pathlib import Path

import pytest

pytestmark = pytest.mark.system


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _system_env() -> dict[str, str]:
    root = _repo_root()
    env = os.environ.copy()
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    env["PYTHONPATH"] = str(root / "compiler" / "frontend")
    return env


def _require_system_prereqs() -> dict[str, str]:
    env = _system_env()
    pycc = env.get("PYCC") or shutil.which("pycc")
    toolchain_root = env.get("PYC_TOOLCHAIN_ROOT")
    has_toolchain = bool(toolchain_root) or bool(pycc)
    if not has_toolchain:
        pytest.skip("system test requires PYCC or PYC_TOOLCHAIN_ROOT")
    if shutil.which("verilator") is None:
        pytest.skip("system test requires verilator")
    return env


def test_counter_build_smoke_runs_cpp_and_verilator(tmp_path: Path) -> None:
    env = _require_system_prereqs()
    root = _repo_root()
    out_dir = tmp_path / "counter_build"
    cmd = [
        sys.executable,
        "-m",
        "pycircuit.cli",
        "build",
        str(root / "designs" / "examples" / "counter" / "tb_counter.py"),
        "--out-dir",
        str(out_dir),
        "--target",
        "both",
        "--jobs",
        "1",
        "--logic-depth",
        "64",
        "--run-verilator",
    ]
    subprocess.run(cmd, cwd=root, env=env, check=True)
    assert (out_dir / "project_manifest.json").is_file()


def test_trace_dsl_build_emits_probe_manifest(tmp_path: Path) -> None:
    env = _require_system_prereqs()
    root = _repo_root()
    out_dir = tmp_path / "trace_dsl_build"
    cmd = [
        sys.executable,
        "-m",
        "pycircuit.cli",
        "build",
        str(
            root / "designs" / "examples" / "trace_dsl_smoke" / "tb_trace_dsl_smoke.py"
        ),
        "--out-dir",
        str(out_dir),
        "--target",
        "both",
        "--jobs",
        "1",
        "--logic-depth",
        "64",
        "--trace-config",
        str(
            root
            / "designs"
            / "examples"
            / "trace_dsl_smoke"
            / "trace_dsl_smoke_trace.json"
        ),
    ]
    subprocess.run(cmd, cwd=root, env=env, check=True)

    manifest = json.loads((out_dir / "probe_manifest.json").read_text(encoding="utf-8"))
    probes = manifest.get("probes", [])
    assert any(
        str(probe.get("canonical_path", "")).endswith(":probe.pv.q") for probe in probes
    )


def test_semantic_regressions_script_passes() -> None:
    env = _require_system_prereqs()
    root = _repo_root()
    run_id = f"pytest-system-{uuid.uuid4().hex[:8]}"
    env["PYC_GATE_RUN_ID"] = run_id

    subprocess.run(
        ["bash", "flows/scripts/run_semantic_regressions_v6.sh"],
        cwd=root,
        env=env,
        check=True,
    )

    summary_path = (
        root / "docs" / "gates" / "logs" / run_id / "semantic_regressions_summary.json"
    )
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    assert summary["status"] == "pass"
