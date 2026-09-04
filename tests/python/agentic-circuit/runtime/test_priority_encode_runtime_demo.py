from __future__ import annotations

from pathlib import Path
import pytest


ROOT = Path(__file__).resolve().parents[4]
DEMO = ROOT / "examples" / "agentic-circuit" / "blocks" / "priority_encode_runtime"
ARTIFACTS = ROOT / ".pycircuit_out" / "examples" / "priority_encode_runtime"


def test_priority_encode_demo_artifacts_are_published() -> None:
    if not (ARTIFACTS / "priority_encode.ac.mlir").exists():
        pytest.skip("run the priority encoder demo to populate .pycircuit_out")
    acir = (ARTIFACTS / "priority_encode.ac.mlir").read_text()
    pyc = (ARTIFACTS / "priority_encode.pyc.mlir").read_text()
    verilog = (ARTIFACTS / "priority_encode.generated.v").read_text()
    assert "ac.var.priority_encode" in acir
    assert 'primitive_id = "encoding-arbitration.basejump-priority.v1"' in acir
    assert "pyc.priority_encode" in pyc
    assert "github.bespoke-silicon-group.basejump_stl.bsg_priority_encode" in pyc
    assert "pyc_runtime_basejump_priority_encode" in verilog
    assert "bsg_priority_encode" in verilog


def test_priority_encode_demo_gate_report_is_green() -> None:
    import json

    if not (ARTIFACTS / "priority_encode.gates.json").exists():
        pytest.skip("run the priority encoder demo to populate .pycircuit_out")
    report = json.loads(
        (ARTIFACTS / "priority_encode.gates.json").read_text()
    )
    assert report["status"] == "passed"
    assert report["gates"] == {"verilator": True, "yosys": True}
