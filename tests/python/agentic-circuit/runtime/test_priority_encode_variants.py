from __future__ import annotations

import json
from pathlib import Path
import pytest


ROOT = Path(__file__).resolve().parents[4]
DEMO = ROOT / "examples" / "agentic-circuit" / "blocks" / "priority_encode_runtime"
VARIANTS = ROOT / ".pycircuit_out" / "examples" / "priority_encode_runtime" / "variants"


def test_priority_encode_variants_have_distinct_ir_and_rtl() -> None:
    if not (VARIANTS / "w8_lo_to_hi" / "priority_encode.ac.mlir").exists():
        pytest.skip("run the priority encoder variants demo to populate .pycircuit_out")
    w8_acir = (VARIANTS / "w8_lo_to_hi" / "priority_encode.ac.mlir").read_text()
    w16_acir = (VARIANTS / "w16_hi_to_lo" / "priority_encode.ac.mlir").read_text()
    w8_pyc = (VARIANTS / "w8_lo_to_hi" / "priority_encode.pyc.mlir").read_text()
    w16_pyc = (VARIANTS / "w16_hi_to_lo" / "priority_encode.pyc.mlir").read_text()
    w8_v = (VARIANTS / "w8_lo_to_hi" / "priority_encode.generated.v").read_text()
    w16_v = (VARIANTS / "w16_hi_to_lo" / "priority_encode.generated.v").read_text()

    assert "!ac.var<i8> -> !ac.var<i4>" in w8_acir
    assert "lo_to_hi = true" in w8_acir
    assert "!ac.var<i16> -> !ac.var<i5>" in w16_acir
    assert "lo_to_hi = false" in w16_acir
    assert "width = 8, lo_to_hi = 1" in w8_pyc
    assert "width = 16, lo_to_hi = 0" in w16_pyc
    assert ".WIDTH(8), .LO_TO_HI(1)" in w8_v
    assert ".WIDTH(16), .LO_TO_HI(0)" in w16_v


def test_priority_encode_variants_gate_report_is_green() -> None:
    if not (VARIANTS.parent / "priority_encode_variants.gates.json").exists():
        pytest.skip("run the priority encoder variants demo to populate .pycircuit_out")
    report = json.loads(
        (VARIANTS.parent / "priority_encode_variants.gates.json").read_text()
    )
    assert report["status"] == "passed"
    assert [entry["configuration"]["name"] for entry in report["configurations"]] == [
        "w8_lo_to_hi",
        "w16_hi_to_lo",
    ]
    assert all(entry["gates"] == {"verilator": True, "yosys": True} for entry in report["configurations"])
