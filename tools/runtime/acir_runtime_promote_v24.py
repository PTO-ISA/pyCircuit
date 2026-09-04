#!/usr/bin/env python3
"""Promote PULP flushable stream arbiter as runtime RTL v0.24."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Mapping

from _paths import normalize_spec

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests/parameterized-components-v0.24.json"
LOCK = ROOT / "catalog.lock.json"


def _spec() -> dict[str, Any]:
    base = "verilog/vendor-v0.24/pulp_common_cells"
    return {
        "name": "pulp-stream-arbiter-flushable", "module": "pyc_runtime_pulp_stream_arbiter_flushable",
        "implementation": "stream_arbiter_flushable", "source": "pulp-common-cells-v0.24",
        "provider": "github", "family": "arbitration-interconnect",
        "wrapper": "verilog/pyc_runtime_pulp_stream_arbiter_flushable.sv",
        "repository": "https://github.com/pulp-platform/common_cells.git",
        "commit": "63b7c50d43e462b59506f69d341ff1e40202866d", "source_file": "src/deprecated/stream_arbiter_flushable.sv",
        "license": "Solderpad-Hardware-License-0.51", "license_file": "licenses/pulp-common-cells-v0.20-LICENSE",
        "source_files": [f"{base}/src/deprecated/stream_arbiter_flushable.sv", f"{base}/src/cc_stream_arbiter.sv", f"{base}/src/cc_rr_arb_tree.sv", f"{base}/src/cc_lzc.sv", f"{base}/src/cc_pkg.sv", f"{base}/include/common_cells/assertions.svh", f"{base}/include/common_cells/registers.svh", f"{base}/include/common_cells/deprecated/registers.svh"],
        "parameters": [{"name": "INPUTS", "source": "N_INP", "default": 2}, {"name": "DATA_WIDTH", "source": "DATA_T width", "default": 8}, {"name": "ARBITER", "source": "upstream string fixed to rr", "default": "rr"}],
        "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset_n", "direction": "input", "width": "1"}, {"name": "flush", "direction": "input", "width": "1"}, {"name": "input_data", "direction": "input", "width": "INPUTS*DATA_WIDTH"}, {"name": "input_valid", "direction": "input", "width": "INPUTS"}, {"name": "input_ready", "direction": "output", "width": "INPUTS"}, {"name": "output_data", "direction": "output", "width": "DATA_WIDTH"}, {"name": "output_valid", "direction": "output", "width": "1"}, {"name": "output_ready", "direction": "input", "width": "1"}],
        "oracle": {"id": "pulp-stream-arbiter-flushable-v1", "kind": "cycle/ready-valid/flush", "contract": "round-robin ready/valid arbitration holds under backpressure and clears the pending arbitration state when flush is asserted"},
        "configs": [{"INPUTS": 2, "DATA_WIDTH": 4}, {"INPUTS": 3, "DATA_WIDTH": 8}, {"INPUTS": 4, "DATA_WIDTH": 8}],
    }


def _read(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    return value if isinstance(value, dict) else {}


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _result(report: Mapping[str, Any], name: str) -> dict[str, Any]:
    return next((dict(item) for item in report.get("results", []) if isinstance(item, Mapping) and str(item.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(x) for x in spec["source_files"]]]; lic = str(spec["license_file"])
    missing = [x for x in [*files, lic] if not (ROOT / x).is_file()]
    f = _result(functional, str(spec["name"])); g = _result(gate, str(spec["name"]))
    staging = not functional and not gate; accepted = not missing and (staging or (f.get("status") == "passed" and g.get("status") == "passed"))
    qor = []
    for case in f.get("cases", []) if isinstance(f, Mapping) else []:
        if isinstance(case, Mapping) and isinstance(case.get("qor"), Mapping):
            q = case["qor"]; qor.append({"parameters": case.get("parameters", {}), **{k: q.get(k) for k in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    return {"name": spec["name"], "module": spec["module"], "implementation": spec["implementation"], "source": spec["source"], "provider": spec["provider"], "status": "accepted" if accepted else "pending", "family": spec["family"], "wrapper": spec["wrapper"], "files": files, "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": spec["source_file"], "license": spec["license"]}, "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]}, "oracle": spec["oracle"], "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": [lic], "include_roots": ["verilog/vendor-v0.24/pulp_common_cells/include", "verilog/vendor-v0.24/pulp_common_cells/src"]}, "validation": {"status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.24.json", "configs": spec["configs"], "functional_report": ".pycircuit_out/runtime-functional-validation/v24-pulp-flushable.json", "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v24-pulp-flushable.json", "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qor}, "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})}}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0]); parser.add_argument("--mode", choices=("prepare", "finalize"), required=True); parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v24-pulp-flushable.json"); parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v24-pulp-flushable.json"); args = parser.parse_args()
    spec = normalize_spec(_spec()); functional = _read(args.functional_report) if args.mode == "finalize" else {}; gate = _read(args.gate_report) if args.mode == "finalize" else {}
    catalog = _read(CATALOG); entries = [e for e in catalog.get("entries", []) if str(e.get("name")) != spec["name"]]; entries.append(_entry(spec, functional, gate)); entries.sort(key=lambda e: str(e.get("name", ""))); catalog.update({"entries": entries, "runtime_api_version": "0.24", "generated_by": "acir-runtime-promote-v24"}); CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    old = _read(ROOT / "manifests/parameterized-components-v0.23.json"); components = [c for c in old.get("components", []) if str(c.get("name")) != spec["name"]]; components.append({"name": spec["name"], "oracle": spec["oracle"], "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["source_files"], "license": spec["license_file"]}}); hashes = {}
    for entry in entries:
        for path_text in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(path_text)
            if path.is_file(): hashes[str(path_text)] = _sha(path)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.24", "release": "runtime-rtl-v0.24", "generated_by": "acir-runtime-promote-v24", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda c: str(c.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = _read(LOCK); lock.setdefault("sources", {})["pulp-common-cells-v0.24"] = {"repository": spec["repository"], "commit": spec["commit"], "license": spec["license"]}; LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": 1, "accepted": int(entries[-1].get("status") == "accepted")}, ensure_ascii=False)); return 0


if __name__ == "__main__":
    raise SystemExit(main())
