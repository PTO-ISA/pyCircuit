from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit

EXPECTED_EXAMPLES = {
    "arith",
    "bf16_fmac",
    "boundary_value_ports",
    "cache_params",
    "calculator",
    "counter",
    "decode_rules",
    "digital_clock",
    "digital_filter",
    "dodgeball_game",
    "fastfwd",
    "fifo_loopback",
    "hier_modules",
    "issue_queue_2picker",
    "jit_control_flow",
    "jit_pipeline_vec",
    "mem_rdw_olddata",
    "multiclock_regs",
    "net_resolution_depth_smoke",
    "obs_points",
    "pipeline_builder",
    "reset_invalidate_order_smoke",
    "struct_transform",
    "sync_mem_init_zero",
    "trace_dsl_smoke",
    "traffic_lights_ce_pyc",
    "wire_ops",
    "xz_value_model_smoke",
}


def test_all_public_pycircuit_examples_are_discovered_and_classified() -> None:
    completed = subprocess.run(
        (
            sys.executable,
            "flows/tools/discover_examples.py",
            "--root",
            "examples/pycircuit",
            "--format",
            "json",
        ),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    cases = json.loads(completed.stdout)

    assert {case["name"] for case in cases} == EXPECTED_EXAMPLES
    assert {case["category"] for case in cases} == {
        "applications",
        "basics",
        "features",
    }
    for case in cases:
        assert Path(case["design"]).is_file()
        assert Path(case["tb"]).is_file()
        assert Path(case["config"]).is_file()


def test_designs_root_and_generated_profiles_are_not_tracked() -> None:
    tracked = subprocess.run(
        ("git", "ls-files"),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()

    assert not any(path == "designs" or path.startswith("designs/") for path in tracked)
    assert not any(path.endswith((".profraw", ".profdata")) for path in tracked)
    assert not any(
        path.startswith(("examples/", "benchmarks/"))
        and path.endswith((".a", ".dylib", ".o", ".pyc", ".so"))
        for path in tracked
    )


def test_every_public_example_emits_canonical_pyc(tmp_path: Path) -> None:
    discovered = subprocess.run(
        (
            sys.executable,
            "flows/tools/discover_examples.py",
            "--root",
            "examples/pycircuit",
            "--format",
            "json",
        ),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    )
    environment = os.environ.copy()
    environment["PYTHONDONTWRITEBYTECODE"] = "1"
    environment["PYTHONPATH"] = os.pathsep.join(
        (
            str(ROOT / "python/semantic-core/src"),
            str(ROOT / "python/pycircuit/src"),
        )
    )
    for case in json.loads(discovered.stdout):
        output = tmp_path / f"{case['name']}.pyc"
        emitted = subprocess.run(
            (
                sys.executable,
                "-m",
                "pycircuit.cli",
                "emit",
                case["design"],
                "-o",
                output,
            ),
            cwd=ROOT,
            env=environment,
            text=True,
            capture_output=True,
            check=False,
        )
        assert (
            emitted.returncode == 0
        ), f"{case['name']} failed:\n{emitted.stdout}\n{emitted.stderr}"
        assert "pyc.frontend.contract" in output.read_text(encoding="utf-8")
