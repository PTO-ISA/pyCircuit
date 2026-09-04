#!/usr/bin/env python3
"""Run the bounded OpenTitan one-hot mux promotion gate."""
from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from acir_runtime_functional import run_case


ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / "library/verilog"
VERILATOR = "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator"
YOSYS = "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys"


def main() -> int:
    wrapper = RUNTIME / "pyc_runtime_opentitan_onehot_mux.sv"
    source = RUNTIME / "opentitan/prim_onehot_mux.sv"
    and_source = RUNTIME / "opentitan/prim_and2.sv"
    assertion = RUNTIME / "opentitan/prim_assert.sv"
    name = "opentitan-onehot-mux"
    top = "pyc_runtime_opentitan_onehot_mux"
    configs = [(1, 1), (2, 4), (4, 8), (5, 3)]
    rows = []
    for inputs, width in configs:
        result = run_case(
            name=f"{name}-inputs{inputs}-width{width}",
            kind=name,
            files=[wrapper, source, and_source, assertion],
            verilator=VERILATOR,
            yosys=YOSYS,
            timeout=60,
            num_src=inputs,
            width=width,
            saturate=True,
            dut_name=top,
            yosys_top=top,
        )
        row = {
            "inputs": inputs,
            "width": width,
            "status": result.get("status"),
            "run": result.get("run", {}),
            "synthesis": result.get("synthesis", {}),
            "qor": result.get("qor", {}),
        }
        rows.append(row)
        print(f"{name} INPUTS={inputs} WIDTH={width}: {row['status']}")

    passed = sum(row["status"] == "passed" for row in rows)
    report = {
        "schema": "acir-runtime-functional-validation-v0.1",
        "component": name,
        "toolchain": {"verilator": VERILATOR, "yosys": YOSYS},
        "configs": [{"INPUTS": i, "WIDTH": w} for i, w in configs],
        "summary": {
            "entries": len(rows),
            "status": "passed" if passed == len(rows) else "failed",
            "counts": {"passed": passed, "failed": len(rows) - passed},
        },
        "results": rows,
    }
    out = ROOT / ".pycircuit_out/runtime-functional-validation/v27-opentitan-onehot-mux.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0 if report["summary"]["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
