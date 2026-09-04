from __future__ import annotations

import json
from pathlib import Path

from acir_runtime_adapter import build_adapters


def test_build_adapters_renames_structural_wrapper(tmp_path: Path) -> None:
    validation = tmp_path / "validation"
    case_dir = validation / "cases" / "demo" / "and" / "s0"
    candidate_dir = validation / "candidate-builds" / "demo" / "and"
    case_dir.mkdir(parents=True)
    candidate_dir.mkdir(parents=True)
    (case_dir / "adapter_top.sv").write_text(
        "module pyc_synth_top (\n"
        "  input logic [3:0] a,\n"
        "  output logic [3:0] y\n"
        ");\n"
        "endmodule\n",
        encoding="utf-8",
    )
    (candidate_dir / "manifest.json").write_text(
        json.dumps({"candidate_filelist": "candidate.f", "module_files": ["rtl/and.sv"]}),
        encoding="utf-8",
    )
    report = {
        "case_results": [{
            "case_key": "demo|and|rtl/and.sv|s0",
            "project": "demo", "module": "and", "source_file": "rtl/and.sv",
            "repo_url": "https://example.invalid/demo", "commit_sha": "0123456789abcdef",
            "config": "s0", "closure": "PASS", "verilator_lint": "PASS",
            "simulation": "PASS", "synthesis": "PASS", "parameters": {"WIDTH": 4},
            "mapped_area": 1.0, "logic_depth": 1,
        }],
    }
    promotion = {"entries": [{
        "candidate_id": "1234567890abcdef", "project": "demo", "module": "and",
        "source_file": "rtl/and.sv", "family": "arithmetic", "promotion": "review_required",
        "structural_status": "passed", "semantic_status": "missing", "interface_reviewed": False,
        "license": "MIT", "case_keys": ["demo|and|rtl/and.sv|s0"],
    }]}
    output = tmp_path / "adapters"
    result = build_adapters(report, promotion, validation, output)
    assert result["summary"]["staged_structural"] == 1
    wrapper = next((output / "wrappers").glob("*.sv"))
    text = wrapper.read_text(encoding="utf-8")
    assert "module pyc_candidate_demo_and_12345678" in text
    assert "module pyc_synth_top" not in text
    manifest = next((output / "manifests").glob("*.json"))
    data = json.loads(manifest.read_text(encoding="utf-8"))
    assert data["status"] == "staged_structural"
    assert data["interface_check"]["status"] == "standard_verilog_flat"
    assert data["interface"]["ports"] == [
        {"direction": "input", "name": "a", "width": "[3:0]"},
        {"direction": "output", "name": "y", "width": "[3:0]"},
    ]
