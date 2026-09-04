#!/usr/bin/env python3
"""Run bounded functional/Verilator/Yosys validation for Vortex KS adder."""
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
    wrapper = RUNTIME / "pyc_runtime_vortex_ks_adder.sv"
    source = RUNTIME / "vendor-v0.22/vortex/hw/rtl/libs/VX_ks_adder.sv"
    platform = RUNTIME / "vendor-v0.22/vortex/hw/rtl/VX_platform.vh"
    scope = RUNTIME / "vendor-v0.22/vortex/hw/rtl/VX_scope.vh"
    rows = []
    # saturate=True selects the native Kogge-Stone implementation (BYPASS=0);
    # saturate=False selects the equivalent combinational bypass implementation.
    configs = [(1, 1, True), (4, 4, True), (8, 8, True),
               (13, 13, True), (8, 8, False)]
    for width, num_src, saturate in configs:
        result = run_case(
            name=f"vortex-ks-adder-width{width}-bypass{int(not saturate)}",
            kind="vortex-ks-adder",
            files=[wrapper, source, platform, scope],
            verilator=VERILATOR,
            yosys=YOSYS,
            timeout=60,
            num_src=num_src,
            width=width,
            saturate=saturate,
            dut_name="pyc_runtime_vortex_ks_adder",
            yosys_top="pyc_runtime_vortex_ks_adder",
        )
        rows.append({"WIDTH": width, "BYPASS": int(not saturate), **result})
        print(f"vortex-ks-adder WIDTH={width} BYPASS={int(not saturate)}: {result.get('status')}")
    passed = sum(row.get("status") == "passed" for row in rows)
    report = {
        "schema": "acir-runtime-functional-validation-v0.1",
        "component": "vortex-ks-adder",
        "summary": {
            "entries": len(rows),
            "status": "passed" if passed == len(rows) else "failed",
            "counts": {"passed": passed, "failed": len(rows) - passed},
        },
        "results": rows,
    }
    out = ROOT / ".pycircuit_out/runtime-functional-validation/v29-vortex-ks-adder.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0 if report["summary"]["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
