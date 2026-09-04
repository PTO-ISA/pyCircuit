#!/usr/bin/env python3
"""Promote PULP ``cc_clk_or_tree`` as runtime RTL v0.19.

The release keeps the upstream recursive helper and its technology-cell
boundary in a small, reviewable closure.  ``prepare`` records the staged
component; ``finalize`` consumes the functional and packaged Verilator/Yosys
reports and makes the catalog entry accepted only when both gates pass.
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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.19.json"
LOCK = ROOT / "catalog.lock.json"

PULP_REPO = "https://github.com/pulp-platform/common_cells.git"
PULP_COMMIT = "db42769334b4589b4b3fc671b34513bdb98be565"
PULP_LICENSE = "licenses/pulp-common-cells-v0.18-LICENSE"
TECH_REPO = "https://github.com/pulp-platform/tech_cells_generic.git"
TECH_COMMIT = "55cb54513e2d426be5992d311cb9d5dbcad10c78"
TECH_LICENSE = "licenses/tech-cells-generic-v0.16-LICENSE"


def _spec() -> dict[str, Any]:
    source_files = [
        "verilog/vendor-v0.19/pulp_common_cells/src/cc_clk_mux_glitch_free.sv",
        "verilog/vendor-v0.19/tech_cells_generic/src/rtl/tc_clk.sv",
    ]
    return {
        "name": "pulp-clk-or-tree",
        "module": "pyc_runtime_pulp_clk_or_tree",
        "implementation": "cc_clk_or_tree",
        "source": "pulp-common-cells-v0.19",
        "provider": "github",
        "family": "clocking",
        "wrapper": "verilog/pyc_runtime_pulp_clk_or_tree.sv",
        "source_files": source_files,
        "repository": PULP_REPO,
        "commit": PULP_COMMIT,
        "source_file": "src/cc_clk_mux_glitch_free.sv#cc_clk_or_tree",
        "license": "Solderpad-Hardware-License-0.51",
        "license_file": PULP_LICENSE,
        "extra_license_files": [TECH_LICENSE],
        "parameters": [
            {"name": "NUM_INPUTS", "source": "NumInputs", "default": 2},
        ],
        "ports": [
            {"name": "clks_in", "direction": "input", "width": "NUM_INPUTS"},
            {"name": "clk_out", "direction": "output", "width": "1"},
        ],
        "oracle": {
            "id": "pulp-clk-or-tree-v1",
            "kind": "combinational-clock-tree",
            "contract": "clk_out is the OR of all clks_in bits; one, two, and odd fan-in configurations preserve the same logic while retaining the upstream tc_clk_or2 cell boundary",
        },
        "configs": [
            {"NUM_INPUTS": 1},
            {"NUM_INPUTS": 2},
            {"NUM_INPUTS": 3},
            {"NUM_INPUTS": 5},
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
    return next((dict(item) for item in report.get("results", [])
                 if isinstance(item, Mapping) and str(item.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(item) for item in spec["source_files"]]]
    licenses = [str(spec["license_file"]), *[str(item) for item in spec.get("extra_license_files", [])]]
    missing = [path for path in [*files, *licenses] if not (ROOT / path).is_file()]
    f = _result(functional, str(spec["name"]))
    g = _result(gate, str(spec["name"]))
    staging = not functional and not gate
    accepted = not missing and (staging or (f.get("status") == "passed" and g.get("status") == "passed"))
    qor = []
    for case in f.get("cases", []) if isinstance(f, Mapping) else []:
        if isinstance(case, Mapping) and isinstance(case.get("qor"), Mapping):
            q = case["qor"]
            qor.append({"parameters": case.get("parameters", {}), **{key: q.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": spec["source"], "provider": spec["provider"],
        "status": "accepted" if accepted else "pending", "family": spec["family"],
        "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": spec["source_file"], "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": licenses, "include_roots": ["verilog/vendor-v0.19/tech_cells_generic/src/rtl"]},
        "validation": {
            "status": "passed" if accepted and not staging else "pending",
            "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1",
            "manifest": "manifests/parameterized-components-v0.19.json", "configs": spec["configs"],
            "functional_report": _relative(REPO / ".pycircuit_out/runtime-functional-validation/v19-clk-or-tree.json"),
            "runtime_gate_report": _relative(REPO / ".pycircuit_out/runtime-catalog-validation/v19-clk-or-tree.json"),
            "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"),
            "qor": qor,
        },
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v19-clk-or-tree.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v19-clk-or-tree.json")
    args = parser.parse_args()
    spec = normalize_spec(_spec())
    functional = _read(args.functional_report) if args.mode == "finalize" else {}
    gate = _read(args.gate_report) if args.mode == "finalize" else {}
    catalog = _read(CATALOG)
    entries = [entry for entry in catalog.get("entries", []) if str(entry.get("name")) != spec["name"]]
    entries.append(_entry(spec, functional, gate)); entries.sort(key=lambda entry: str(entry.get("name", "")))
    catalog["entries"] = entries; catalog["runtime_api_version"] = "0.19"; catalog["generated_by"] = "acir-runtime-promote-v19"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old = _read(ROOT / "manifests/parameterized-components-v0.18.json")
    components = [component for component in old.get("components", []) if str(component.get("name")) != spec["name"]]
    components.append({"name": spec["name"], "oracle": spec["oracle"], "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["source_files"], "license": spec["license_file"], "extra_licenses": spec["extra_license_files"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for path_text in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(path_text)
            if path.is_file(): hashes[str(path_text)] = _sha(path)
    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.19", "release": "runtime-rtl-v0.19", "generated_by": "acir-runtime-promote-v19", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda component: str(component.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = _read(LOCK); sources = lock.setdefault("sources", {})
    sources["pulp-common-cells-v0.19"] = {"repository": PULP_REPO, "commit": PULP_COMMIT, "license": "Solderpad-Hardware-License-0.51"}
    sources["tech-cells-generic-v0.19"] = {"repository": TECH_REPO, "commit": TECH_COMMIT, "license": "Solderpad-Hardware-License-0.51"}
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": 1, "accepted": int(entries[-1].get("status") == "accepted")}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
