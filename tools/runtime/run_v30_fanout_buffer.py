#!/usr/bin/env python3
"""Run bounded functional/Verilator/Yosys validation for Vortex fanout buffer."""
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
    wrapper = RUNTIME / "pyc_runtime_vortex_fanout_buffer.sv"
    source = RUNTIME / "vendor-v0.22/vortex/hw/rtl/libs/VX_fanout_buffer.sv"
    platform = RUNTIME / "vendor-v0.22/vortex/hw/rtl/VX_platform.vh"
    scope = RUNTIME / "vendor-v0.22/vortex/hw/rtl/VX_scope.vh"
    configs = [(1, 8), (4, 8), (8, 8), (13, 8), (16, 0)]
    rows = []
    for outputs, max_fanout in configs:
        result = run_case(
            name=f"vortex-fanout-buffer-outputs{outputs}-fanout{max_fanout}",
            kind="vortex-fanout-buffer",
            files=[wrapper, source, platform, scope],
            verilator=VERILATOR,
            yosys=YOSYS,
            timeout=60,
            num_src=outputs,
            width=max_fanout,
            saturate=True,
            dut_name="pyc_runtime_vortex_fanout_buffer",
            yosys_top="pyc_runtime_vortex_fanout_buffer",
        )
        row = {"OUTPUTS": outputs, "MAX_FANOUT": max_fanout, **result}
        rows.append(row)
        print(f"vortex-fanout-buffer OUTPUTS={outputs} MAX_FANOUT={max_fanout}: {row.get('status')}")
    passed = sum(row.get("status") == "passed" for row in rows)
    report = {
        "schema": "acir-runtime-functional-validation-v0.1",
        "component": "vortex-fanout-buffer",
        "summary": {"entries": len(rows), "status": "passed" if passed == len(rows) else "failed",
                    "counts": {"passed": passed, "failed": len(rows) - passed}},
        "results": rows,
    }
    out = ROOT / ".pycircuit_out/runtime-functional-validation/v30-vortex-fanout-buffer.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0 if report["summary"]["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
