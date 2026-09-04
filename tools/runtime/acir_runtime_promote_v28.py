#!/usr/bin/env python3
"""Promote BaseJump's iterative multiplier as runtime RTL v0.28."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from _paths import normalize_spec

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests/parameterized-components-v0.28.json"
LOCK = ROOT / "catalog.lock.json"


def main() -> int:
    name = "basejump-imul-iterative"
    wrapper = "verilog/pyc_runtime_basejump_imul_iterative.sv"
    source = "verilog/basejump/bsg_imul_iterative.sv"
    defines = "verilog/basejump/bsg_defines.sv"
    license_file = "licenses/basejump-stl/LICENSE"
    files = [wrapper, source, defines]
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    entry = normalize_spec({
        "name": name,
        "module": "pyc_runtime_basejump_imul_iterative",
        "implementation": "bsg_imul_iterative",
        "source": "basejump-stl-v0.28",
        "provider": "github",
        "status": "accepted",
        "family": "arithmetic-iterative",
        "wrapper": wrapper,
        "files": files,
        "provenance": {
            "repository": "https://github.com/bespoke-silicon-group/basejump_stl.git",
            "commit": "b48037e28544425839dbd617d45b1a82631bc1a9",
            "source_file": "bsg_misc/bsg_imul_iterative.sv",
            "license": "Solderpad-Hardware-License-0.51",
        },
        "interface": {
            "wrapper_module": "pyc_runtime_basejump_imul_iterative",
            "parameters": [{"name": "WIDTH", "source": "width_p", "default": 8}],
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "rst", "direction": "input", "width": "1"},
                {"name": "in_valid", "direction": "input", "width": "1"},
                {"name": "in_ready", "direction": "output", "width": "1"},
                {"name": "op_a", "direction": "input", "width": "WIDTH"},
                {"name": "signed_a", "direction": "input", "width": "1"},
                {"name": "op_b", "direction": "input", "width": "WIDTH"},
                {"name": "signed_b", "direction": "input", "width": "1"},
                {"name": "high_part", "direction": "input", "width": "1"},
                {"name": "out_valid", "direction": "output", "width": "1"},
                {"name": "result", "direction": "output", "width": "WIDTH"},
                {"name": "out_ready", "direction": "input", "width": "1"},
            ],
        },
        "oracle": {
            "id": "basejump-imul-iterative-v1",
            "kind": "cycle/ready-valid",
            "contract": "accept an unsigned or signed WIDTH-bit operand pair when in_ready is asserted, produce the low or high WIDTH-bit product selected by high_part, hold it while out_ready is low, and return to idle after consumption",
        },
        "dependency_closure": {
            "status": "complete",
            "source_files": [source, defines],
            "license_files": [license_file],
            "include_roots": ["verilog/basejump"],
        },
        "validation": {
            "status": "passed",
            "mode": "packaged-functional-verilator-yosys",
            "semantic_status": "functional_oracle_v1",
            "manifest": "manifests/parameterized-components-v0.28.json",
            "configs": [{"WIDTH": 4}, {"WIDTH": 8}],
            "functional_report": ".pycircuit_out/runtime-functional-validation/v28-basejump-imul-iterative.json",
            "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v28-basejump-imul-iterative.json",
        },
    })
    entries = [e for e in catalog.get("entries", []) if e.get("name") != name]
    entries.append(entry)
    entries.sort(key=lambda e: str(e.get("name", "")))
    catalog.update({"entries": entries, "runtime_api_version": "0.28", "generated_by": "acir-runtime-promote-v28"})
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    previous = json.loads((ROOT / "manifests/parameterized-components-v0.27.json").read_text(encoding="utf-8"))
    components = [c for c in previous.get("components", []) if c.get("name") != name]
    components.append({
        "name": name,
        "oracle": entry["oracle"],
        "parameters": {"WIDTH": "width_p"},
        "configs": entry["validation"]["configs"],
        "source": {"repository": entry["provenance"]["repository"], "commit": entry["provenance"]["commit"], "files": files, "license": license_file},
    })
    hashes = {}
    for item in entries:
        for path_text in [*item.get("files", []), *item.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / path_text
            if path.is_file():
                hashes[path_text] = hashlib.sha256(path.read_bytes()).hexdigest()
    MANIFEST.write_text(json.dumps({
        "schema": "acir-runtime-parameterized-components-v0.28",
        "release": "runtime-rtl-v0.28",
        "generated_by": "acir-runtime-promote-v28",
        "toolchain": previous.get("toolchain", {}),
        "policy": previous.get("policy", {}),
        "components": sorted(components, key=lambda c: str(c.get("name", ""))),
        "sha256": dict(sorted(hashes.items())),
    }, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    lock.setdefault("sources", {})["basejump-stl-v0.28"] = {
        "repository": entry["provenance"]["repository"],
        "commit": entry["provenance"]["commit"],
        "license": entry["provenance"]["license"],
    }
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"new_components": 1, "catalog_entries": len(entries), "manifest_components": len(components)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
