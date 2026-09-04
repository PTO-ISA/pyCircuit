#!/usr/bin/env python3
"""Promote Vortex's combinational fanout buffer as runtime RTL v0.30."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from _paths import normalize_spec

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests/parameterized-components-v0.30.json"
LOCK = ROOT / "catalog.lock.json"


def main() -> int:
    name = "vortex-fanout-buffer"
    wrapper = "verilog/pyc_runtime_vortex_fanout_buffer.sv"
    source = "verilog/vendor-v0.22/vortex/hw/rtl/libs/VX_fanout_buffer.sv"
    platform = "verilog/vendor-v0.22/vortex/hw/rtl/VX_platform.vh"
    scope = "verilog/vendor-v0.22/vortex/hw/rtl/VX_scope.vh"
    license_file = "licenses/vortex-v0.13-LICENSE"
    files = [wrapper, source, platform, scope]
    configs = [{"OUTPUTS": o, "MAX_FANOUT": f} for o, f in [(1, 8), (4, 8), (8, 8), (13, 8), (16, 0)]]
    oracle = {
        "id": "vortex-fanout-buffer-v1",
        "kind": "combinational-replication",
        "contract": "replicate data_in identically to every bit of data_out; OUTPUTS controls vector width and MAX_FANOUT controls only the internal split-vs-passthrough implementation",
    }
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    entry = normalize_spec({
        "name": name,
        "module": "pyc_runtime_vortex_fanout_buffer",
        "implementation": "VX_fanout_buffer",
        "source": "vortex-v0.22",
        "provider": "github",
        "status": "accepted",
        "family": "wiring-control",
        "wrapper": wrapper,
        "files": files,
        "provenance": {"repository": "https://github.com/vortexgpgpu/vortex.git",
                       "commit": "d76b7f24e658867ab57e3942d7c648c3e6af072d",
                       "source_file": "hw/rtl/libs/VX_fanout_buffer.sv", "license": "Apache-2.0"},
        "interface": {
            "wrapper_module": "pyc_runtime_vortex_fanout_buffer",
            "parameters": [{"name": "OUTPUTS", "source": "N", "default": 1},
                           {"name": "MAX_FANOUT", "source": "MAX_FANOUT", "default": 8}],
            "ports": [{"name": "data_in", "direction": "input", "width": "1"},
                      {"name": "data_out", "direction": "output", "width": "OUTPUTS"}],
        },
        "oracle": oracle,
        "dependency_closure": {"status": "complete", "source_files": [source, platform, scope],
                                "license_files": [license_file],
                                "include_roots": ["verilog/vendor-v0.22/vortex/hw",
                                                  "verilog/vendor-v0.22/vortex/hw/rtl",
                                                  "verilog/vendor-v0.22/vortex/hw/rtl/libs"]},
        "validation": {"status": "passed", "mode": "packaged-functional-verilator-yosys",
                        "semantic_status": "functional_oracle_v1",
                        "manifest": "manifests/parameterized-components-v0.30.json",
                        "configs": configs,
                        "functional_report": ".pycircuit_out/runtime-functional-validation/v30-vortex-fanout-buffer.json",
                        "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v30-vortex-fanout-buffer.json"},
    })
    entries = [e for e in catalog.get("entries", []) if e.get("name") != name] + [entry]
    entries.sort(key=lambda e: str(e.get("name", "")))
    catalog.update({"entries": entries, "runtime_api_version": "0.30", "generated_by": "acir-runtime-promote-v30"})
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    previous = json.loads((ROOT / "manifests/parameterized-components-v0.29.json").read_text(encoding="utf-8"))
    components = [c for c in previous.get("components", []) if c.get("name") != name]
    components.append({"name": name, "oracle": oracle, "parameters": {"OUTPUTS": "N", "MAX_FANOUT": "MAX_FANOUT"},
                       "configs": configs, "source": {"repository": entry["provenance"]["repository"],
                       "commit": entry["provenance"]["commit"], "files": files, "license": license_file}})
    hashes = {}
    for item in entries:
        for path_text in [*item.get("files", []), *item.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / path_text
            if path.is_file(): hashes[path_text] = hashlib.sha256(path.read_bytes()).hexdigest()
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.30",
        "release": "runtime-rtl-v0.30", "generated_by": "acir-runtime-promote-v30",
        "toolchain": previous.get("toolchain", {}), "policy": previous.get("policy", {}),
        "components": sorted(components, key=lambda c: str(c.get("name", ""))),
        "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    lock.setdefault("sources", {})["vortex-v0.22"] = {"repository": entry["provenance"]["repository"],
        "commit": entry["provenance"]["commit"], "license": entry["provenance"]["license"]}
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"new_components": 1, "catalog_entries": len(entries), "manifest_components": len(components)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
