#!/usr/bin/env python3
"""Promote PULP ``cc_fall_through_register`` as runtime RTL v0.20."""

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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.20.json"
LOCK = ROOT / "catalog.lock.json"

PULP_REPO = "https://github.com/pulp-platform/common_cells.git"
PULP_COMMIT = "db42769334b4589b4b3fc671b34513bdb98be565"
PULP_LICENSE = "licenses/pulp-common-cells-v0.20-LICENSE"


def _spec() -> dict[str, Any]:
    v = "verilog/vendor-v0.20/pulp_common_cells"
    source_files = [
        f"{v}/src/cc_fall_through_register.sv",
        f"{v}/src/cc_stream_fifo.sv",
        f"{v}/src/cc_fifo.sv",
        f"{v}/src/cc_pkg.sv",
        f"{v}/include/common_cells/registers.svh",
        f"{v}/include/common_cells/assertions.svh",
        f"{v}/include/common_cells/deprecated/registers.svh",
    ]
    return {
        "name": "pulp-fall-through-register",
        "module": "pyc_runtime_pulp_fall_through_register",
        "implementation": "cc_fall_through_register",
        "source": "pulp-common-cells-v0.20",
        "provider": "github",
        "family": "storage-dataflow",
        "wrapper": "verilog/pyc_runtime_pulp_fall_through_register.sv",
        "source_files": source_files,
        "repository": PULP_REPO,
        "commit": PULP_COMMIT,
        "source_file": "src/cc_fall_through_register.sv",
        "license": "Solderpad-Hardware-License-0.51",
        "license_file": PULP_LICENSE,
        "parameters": [{"name": "DATA_WIDTH", "source": "data_t width", "default": 8}],
        "ports": [
            {"name": "clk", "direction": "input", "width": "1"},
            {"name": "rst_n", "direction": "input", "width": "1"},
            {"name": "clear", "direction": "input", "width": "1"},
            {"name": "valid_in", "direction": "input", "width": "1"},
            {"name": "ready_in", "direction": "output", "width": "1"},
            {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"},
            {"name": "valid_out", "direction": "output", "width": "1"},
            {"name": "ready_out", "direction": "input", "width": "1"},
            {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"},
        ],
        "oracle": {
            "id": "pulp-fall-through-register-v1",
            "kind": "cycle/ready-valid",
            "contract": "an empty one-entry stage forwards valid/data without a cycle of latency, retains a stalled word exactly once, drains on ready, and returns empty on synchronous clear",
        },
        "configs": [{"DATA_WIDTH": 1}, {"DATA_WIDTH": 4}, {"DATA_WIDTH": 8}, {"DATA_WIDTH": 13}],
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
    licenses = [str(spec["license_file"])]
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
        "source": spec["source"], "provider": spec["provider"], "status": "accepted" if accepted else "pending",
        "family": spec["family"], "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": spec["source_file"], "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": licenses, "include_roots": ["verilog/vendor-v0.20/pulp_common_cells/include"]},
        "validation": {
            "status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys",
            "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.20.json", "configs": spec["configs"],
            "functional_report": _relative(REPO / ".pycircuit_out/runtime-functional-validation/v20-fall-through.json"),
            "runtime_gate_report": _relative(REPO / ".pycircuit_out/runtime-catalog-validation/v20-fall-through.json"),
            "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qor,
        },
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v20-fall-through.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v20-fall-through.json")
    args = parser.parse_args()
    spec = normalize_spec(_spec()); functional = _read(args.functional_report) if args.mode == "finalize" else {}; gate = _read(args.gate_report) if args.mode == "finalize" else {}
    catalog = _read(CATALOG)
    entries = [entry for entry in catalog.get("entries", []) if str(entry.get("name")) != spec["name"]]
    entries.append(_entry(spec, functional, gate)); entries.sort(key=lambda entry: str(entry.get("name", "")))
    catalog["entries"] = entries; catalog["runtime_api_version"] = "0.20"; catalog["generated_by"] = "acir-runtime-promote-v20"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old = _read(ROOT / "manifests/parameterized-components-v0.19.json")
    components = [component for component in old.get("components", []) if str(component.get("name")) != spec["name"]]
    components.append({"name": spec["name"], "oracle": spec["oracle"], "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["source_files"], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for path_text in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(path_text)
            if path.is_file(): hashes[str(path_text)] = _sha(path)
    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.20", "release": "runtime-rtl-v0.20", "generated_by": "acir-runtime-promote-v20", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda component: str(component.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = _read(LOCK); lock.setdefault("sources", {})["pulp-common-cells-v0.20"] = {"repository": PULP_REPO, "commit": PULP_COMMIT, "license": "Solderpad-Hardware-License-0.51"}; LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    promoted = next(entry for entry in entries if str(entry.get("name")) == str(spec["name"]))
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": 1, "accepted": int(promoted.get("status") == "accepted")}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
