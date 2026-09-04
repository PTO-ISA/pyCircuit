#!/usr/bin/env python3
"""Promote PULP's isochronous spill register as runtime v0.18.

The component is kept separate from the asynchronous CDC FIFO family: its
contract requires integer-related clocks and intentionally has no CDC
synchronizers.  ``prepare`` records a staged, complete source closure;
``finalize`` consumes the functional and packaged-gate reports.
"""

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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.18.json"
LOCK = ROOT / "catalog.lock.json"

REPOSITORY = "https://github.com/pulp-platform/common_cells.git"
COMMIT = "db42769334b4589b4b3fc671b34513bdb98be565"
LICENSE = "licenses/pulp-common-cells-v0.18-LICENSE"


def _spec() -> dict[str, Any]:
    v = "verilog/vendor-v0.18/pulp_common_cells"
    return {
        "name": "pulp-isochronous-spill-register",
        "module": "pyc_runtime_pulp_isochronous_spill_register",
        "implementation": "cc_isochronous_spill_register",
        "source": "pulp-common-cells-v0.18",
        "provider": "github",
        "family": "clock-domain-dataflow",
        "wrapper": "verilog/pyc_runtime_pulp_isochronous_spill_register.sv",
        "source_files": [
            f"{v}/src/cc_isochronous_spill_register.sv",
            f"{v}/include/common_cells/registers.svh",
            f"{v}/include/common_cells/assertions.svh",
            f"{v}/include/common_cells/deprecated/registers.svh",
        ],
        "repository": REPOSITORY,
        "commit": COMMIT,
        "source_file": "src/cc_isochronous_spill_register.sv",
        "license": "Solderpad-Hardware-License-0.51",
        "license_file": LICENSE,
        "parameters": [
            {"name": "DATA_WIDTH", "source": "data_t width", "default": 8},
            {"name": "BYPASS", "source": "Bypass", "default": 0},
        ],
        "ports": [
            {"name": "src_clk", "direction": "input", "width": "1"},
            {"name": "src_rst_n", "direction": "input", "width": "1"},
            {"name": "src_valid", "direction": "input", "width": "1"},
            {"name": "src_ready", "direction": "output", "width": "1"},
            {"name": "src_data", "direction": "input", "width": "DATA_WIDTH"},
            {"name": "dst_clk", "direction": "input", "width": "1"},
            {"name": "dst_rst_n", "direction": "input", "width": "1"},
            {"name": "dst_valid", "direction": "output", "width": "1"},
            {"name": "dst_ready", "direction": "input", "width": "1"},
            {"name": "dst_data", "direction": "output", "width": "DATA_WIDTH"},
        ],
        "oracle": {
            "id": "pulp-isochronous-spill-register-v1",
            "kind": "cycle/related-clock",
            "contract": "with integer-related source/destination clocks, ready/valid transfers preserve data exactly once; BYPASS exposes the documented zero-latency path",
        },
        "configs": [
            {"DATA_WIDTH": 4, "BYPASS": 0},
            {"DATA_WIDTH": 8, "BYPASS": 1},
        ],
    }


def _read(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    return value if isinstance(value, dict) else {}


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _relative(path: Path) -> str:
    try:
        return path.relative_to(REPO).as_posix()
    except ValueError:
        return str(path)


def _result(report: Mapping[str, Any], name: str) -> dict[str, Any]:
    return next((dict(x) for x in report.get("results", []) if isinstance(x, Mapping) and str(x.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(x) for x in spec["source_files"]]]
    missing = [p for p in [*files, str(spec["license_file"])] if not (ROOT / p).is_file()]
    f = _result(functional, str(spec["name"]))
    g = _result(gate, str(spec["name"]))
    staging = not functional and not gate
    accepted = not missing and (staging or (f.get("status") == "passed" and g.get("status") == "passed"))
    qor = []
    for case in f.get("cases", []) if isinstance(f, Mapping) else []:
        if isinstance(case, Mapping) and isinstance(case.get("qor"), Mapping):
            q = case["qor"]
            qor.append({"parameters": case.get("parameters", {}), **{k: q.get(k) for k in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": spec["source"], "provider": spec["provider"], "status": "accepted" if accepted else "pending",
        "family": spec["family"], "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": spec["source_file"], "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": [str(spec["license_file"])], "include_roots": ["verilog/vendor-v0.18/pulp_common_cells/include"]},
        "validation": {"status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.18.json", "configs": spec["configs"], "functional_report": _relative(REPO / ".pycircuit_out/runtime-functional-validation/v18-isochronous.json"), "runtime_gate_report": _relative(REPO / ".pycircuit_out/runtime-catalog-validation/v18-isochronous.json"), "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qor},
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v18-isochronous.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v18-isochronous.json")
    args = parser.parse_args()
    spec = normalize_spec(_spec()); name = str(spec["name"]); catalog = _read(CATALOG)
    functional = _read(args.functional_report) if args.mode == "finalize" else {}; gate = _read(args.gate_report) if args.mode == "finalize" else {}
    entries = [e for e in catalog.get("entries", []) if str(e.get("name")) != name]
    entries.append(_entry(spec, functional, gate)); entries.sort(key=lambda e: str(e.get("name", "")))
    catalog["entries"] = entries; catalog["runtime_api_version"] = "0.18"; catalog["generated_by"] = "acir-runtime-promote-v18"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    old = _read(ROOT / "manifests/parameterized-components-v0.17.json")
    components = [c for c in old.get("components", []) if str(c.get("name")) != name]
    components.append({"name": name, "oracle": spec["oracle"], "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["source_files"], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for e in entries:
        for p in [*e.get("files", []), *e.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(p)
            if path.is_file(): hashes[str(p)] = _sha(path)
    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.18", "release": "runtime-rtl-v0.18", "generated_by": "acir-runtime-promote-v18", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda c: str(c.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = _read(LOCK); lock.setdefault("sources", {})["pulp-common-cells-v0.18"] = {"repository": REPOSITORY, "commit": COMMIT, "license": "Solderpad-Hardware-License-0.51"}; LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": 1, "accepted": int(entries[-1].get("status") == "accepted")}, ensure_ascii=False)); return 0


if __name__ == "__main__":
    raise SystemExit(main())
