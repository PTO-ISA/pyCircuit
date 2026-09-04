#!/usr/bin/env python3
"""Promote Vortex's Kogge-Stone adder as runtime RTL v0.29."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from _paths import normalize_spec

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests/parameterized-components-v0.29.json"
LOCK = ROOT / "catalog.lock.json"


def main() -> int:
    name = "vortex-ks-adder"
    wrapper = "verilog/pyc_runtime_vortex_ks_adder.sv"
    source = "verilog/vendor-v0.22/vortex/hw/rtl/libs/VX_ks_adder.sv"
    platform = "verilog/vendor-v0.22/vortex/hw/rtl/VX_platform.vh"
    scope = "verilog/vendor-v0.22/vortex/hw/rtl/VX_scope.vh"
    license_file = "licenses/vortex-v0.13-LICENSE"
    files = [wrapper, source, platform, scope]
    configs = [
        {"WIDTH": 1, "BYPASS": 0},
        {"WIDTH": 4, "BYPASS": 0},
        {"WIDTH": 8, "BYPASS": 0},
        {"WIDTH": 13, "BYPASS": 0},
        {"WIDTH": 8, "BYPASS": 1},
    ]
    oracle = {
        "id": "vortex-ks-adder-v1",
        "kind": "combinational-arithmetic",
        "contract": "produce the WIDTH-bit sum and carry-out of a+b+cin; BYPASS selects the upstream arithmetic implementation while BYPASS=0 exercises the Kogge-Stone prefix network",
    }
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    entry = normalize_spec({
        "name": name,
        "module": "pyc_runtime_vortex_ks_adder",
        "implementation": "VX_ks_adder",
        "source": "vortex-v0.22",
        "provider": "github",
        "status": "accepted",
        "family": "arithmetic-add",
        "wrapper": wrapper,
        "files": files,
        "provenance": {
            "repository": "https://github.com/vortexgpgpu/vortex.git",
            "commit": "d76b7f24e658867ab57e3942d7c648c3e6af072d",
            "source_file": "hw/rtl/libs/VX_ks_adder.sv",
            "license": "Apache-2.0",
        },
        "interface": {
            "wrapper_module": "pyc_runtime_vortex_ks_adder",
            "parameters": [
                {"name": "WIDTH", "source": "N", "default": 16},
                {"name": "BYPASS", "source": "BYPASS", "default": 0},
            ],
            "ports": [
                {"name": "a", "direction": "input", "width": "WIDTH"},
                {"name": "b", "direction": "input", "width": "WIDTH"},
                {"name": "cin", "direction": "input", "width": "1"},
                {"name": "sum", "direction": "output", "width": "WIDTH"},
                {"name": "cout", "direction": "output", "width": "1"},
            ],
        },
        "oracle": oracle,
        "dependency_closure": {
            "status": "complete",
            "source_files": [source, platform, scope],
            "license_files": [license_file],
            "include_roots": [
                "verilog/vendor-v0.22/vortex/hw",
                "verilog/vendor-v0.22/vortex/hw/rtl",
                "verilog/vendor-v0.22/vortex/hw/rtl/libs",
            ],
        },
        "validation": {
            "status": "passed",
            "mode": "packaged-functional-verilator-yosys",
            "semantic_status": "functional_oracle_v1",
            "manifest": "manifests/parameterized-components-v0.29.json",
            "configs": configs,
            "functional_report": ".pycircuit_out/runtime-functional-validation/v29-vortex-ks-adder.json",
            "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v29-vortex-ks-adder.json",
        },
    })
    entries = [entry if e.get("name") == name else e for e in catalog.get("entries", [])]
    if not any(e.get("name") == name for e in entries):
        entries.append(entry)
    entries.sort(key=lambda e: str(e.get("name", "")))
    catalog.update({"entries": entries, "runtime_api_version": "0.29", "generated_by": "acir-runtime-promote-v29"})
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    previous = json.loads((ROOT / "manifests/parameterized-components-v0.28.json").read_text(encoding="utf-8"))
    components = [c for c in previous.get("components", []) if c.get("name") != name]
    components.append({
        "name": name,
        "oracle": oracle,
        "parameters": {"WIDTH": "N", "BYPASS": "BYPASS"},
        "configs": configs,
        "source": {
            "repository": entry["provenance"]["repository"],
            "commit": entry["provenance"]["commit"],
            "files": files,
            "license": license_file,
        },
    })
    hashes = {}
    for item in entries:
        for path_text in [*item.get("files", []), *item.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / path_text
            if path.is_file():
                hashes[path_text] = hashlib.sha256(path.read_bytes()).hexdigest()
    MANIFEST.write_text(json.dumps({
        "schema": "acir-runtime-parameterized-components-v0.29",
        "release": "runtime-rtl-v0.29",
        "generated_by": "acir-runtime-promote-v29",
        "toolchain": previous.get("toolchain", {}),
        "policy": previous.get("policy", {}),
        "components": sorted(components, key=lambda c: str(c.get("name", ""))),
        "sha256": dict(sorted(hashes.items())),
    }, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    lock.setdefault("sources", {})["vortex-v0.22"] = {
        "repository": entry["provenance"]["repository"],
        "commit": entry["provenance"]["commit"],
        "license": entry["provenance"]["license"],
    }
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"new_components": 1, "catalog_entries": len(entries), "manifest_components": len(components)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
