from pathlib import Path
import json
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from acir_runtime_functional import run_case

root = Path("library/verilog").resolve()
results = []
for shape, dw, cw in (("22_16", 16, 22), ("39_32", 32, 39), ("72_64", 64, 72)):
    for inverted in (False, True):
        for decoder in (False, True):
            tag = "inv-hamming-" if inverted else "hamming-"
            direction = "dec" if decoder else "enc"
            top = f"pyc_runtime_opentitan_secded_{'inv_' if inverted else ''}hamming_{shape}_{direction}"
            kind = f"opentitan-secded-{tag}{shape.replace('_', '-')}-{direction}"
            impl_prefix = "prim_secded_inv_hamming_" if inverted else "prim_secded_hamming_"
            impl = f"{impl_prefix}{shape}_{direction}"
            result = run_case(
                name=kind, kind=kind,
                files=[root / f"{top}.sv", root / "opentitan" / f"{impl}.sv"],
                verilator="wsl:/opt/oss-cad/oss-cad-suite/bin/verilator",
                yosys="wsl:/opt/oss-cad/oss-cad-suite/bin/yosys",
                timeout=60, dut_name=top, yosys_top=top,
            )
            row = {"name": kind, "status": result.get("status"),
                   "run": result.get("run", {}).get("status"),
                   "synthesis": result.get("synthesis", {}).get("status"),
                   "qor": result.get("qor", {})}
            results.append(row)
            print(kind, row["status"], row["run"], row["synthesis"])

out = Path(".pycircuit_out/runtime-functional-validation/v26-opentitan-hamming.json")
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps({
    "schema": "acir-runtime-functional-validation-v0.1",
    "summary": {"entries": len(results),
                "status": "passed" if all(x["status"] == "passed" for x in results) else "failed",
                "counts": {"passed": sum(x["status"] == "passed" for x in results),
                           "failed": sum(x["status"] != "passed" for x in results)}},
    "results": results,
}, indent=2), encoding="utf-8")
