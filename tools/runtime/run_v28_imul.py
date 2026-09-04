#!/usr/bin/env python3
"""Run bounded functional/synthesis validation for BaseJump iterative multiply."""
from __future__ import annotations
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from acir_runtime_functional import run_case

ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / "library/verilog"
VERILATOR = "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator"
YOSYS = "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys"


def main() -> int:
    wrapper = RUNTIME / "pyc_runtime_basejump_imul_iterative.sv"
    source = RUNTIME / "basejump/bsg_imul_iterative.sv"
    defines = RUNTIME / "basejump/bsg_defines.sv"
    rows = []
    for width in (4, 8):
        result = run_case(
            name=f"basejump-imul-iterative-width{width}",
            kind="basejump-imul-iterative",
            files=[wrapper, source, defines],
            verilator=VERILATOR,
            yosys=YOSYS,
            timeout=60,
            num_src=1,
            width=width,
            saturate=True,
            dut_name="pyc_runtime_basejump_imul_iterative",
            yosys_top="pyc_runtime_basejump_imul_iterative",
        )
        rows.append({"WIDTH": width, **result})
        print(f"basejump-imul-iterative WIDTH={width}: {result.get('status')}")
    passed = sum(row.get("status") == "passed" for row in rows)
    report = {
        "schema": "acir-runtime-functional-validation-v0.1",
        "component": "basejump-imul-iterative",
        "summary": {"entries": len(rows), "status": "passed" if passed == len(rows) else "failed",
                    "counts": {"passed": passed, "failed": len(rows) - passed}},
        "results": rows,
    }
    out = ROOT / ".pycircuit_out/runtime-functional-validation/v28-basejump-imul-iterative.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0 if report["summary"]["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
