#!/usr/bin/env python3
"""Promote the next bounded BaseJump runtime batch (v0.12)."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from _paths import normalize_specs

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.12.json"
LOCK = ROOT / "catalog.lock.json"

BASEJUMP_REPO = "https://github.com/bespoke-silicon-group/basejump_stl.git"
BASEJUMP_COMMIT = "b48037e28544425839dbd617d45b1a82631bc1a9"
BASEJUMP_LICENSE = "licenses/basejump-stl-v0.5/LICENSE"
BASEJUMP_VENDOR = "verilog/vendor-v0.11/basejump/bsg_misc"
# bsg_defines is a shared, exact-commit dependency already vendored in v0.5.
BASEJUMP_DEFINES = "verilog/vendor-v0.5/basejump/bsg_misc/bsg_defines.sv"


def _specs() -> list[dict[str, Any]]:
    return [
        {
            "name": "basejump-mux",
            "module": "pyc_runtime_basejump_mux",
            "implementation": "bsg_mux",
            "family": "interconnect",
            "wrapper": "verilog/pyc_runtime_basejump_mux.sv",
            "source_file": "bsg_misc/bsg_mux.sv",
            "source_files": [f"{BASEJUMP_VENDOR}/bsg_mux.sv", BASEJUMP_DEFINES],
            "parameters": [
                {"name": "WIDTH", "source": "width_p", "default": 8},
                {"name": "ELS", "source": "els_p", "default": 2},
                {"name": "HARDEN", "source": "harden_p", "default": 0},
            ],
            "ports": [
                {"name": "data", "direction": "input", "width": "ELS*WIDTH"},
                {"name": "select", "direction": "input", "width": "SELECT_WIDTH"},
                {"name": "out", "direction": "output", "width": "WIDTH"},
            ],
            "oracle": {
                "id": "basejump-mux-v1",
                "kind": "combinational",
                "contract": "out equals data[select] for every valid select in the ELS-element packed input",
            },
            "configs": [
                {"ELS": 1, "WIDTH": 1},
                {"ELS": 2, "WIDTH": 4},
                {"ELS": 4, "WIDTH": 8},
                {"ELS": 5, "WIDTH": 13},
            ],
        },
        {
            "name": "basejump-unconcentrate-static",
            "module": "pyc_runtime_basejump_unconcentrate_static",
            "implementation": "bsg_unconcentrate_static",
            "family": "interconnect",
            "wrapper": "verilog/pyc_runtime_basejump_unconcentrate_static.sv",
            "source_file": "bsg_misc/bsg_unconcentrate_static.sv",
            "source_files": [f"{BASEJUMP_VENDOR}/bsg_unconcentrate_static.sv", BASEJUMP_DEFINES],
            "parameters": [
                {"name": "OUTPUT_ELEMS", "source": "pattern_els_p width", "default": 8},
                {"name": "PATTERN", "source": "pattern_els_p", "default": 255},
            ],
            "ports": [
                {"name": "data", "direction": "input", "width": "INPUT_ELEMS"},
                {"name": "out", "direction": "output", "width": "OUTPUT_ELEMS"},
            ],
            "oracle": {
                "id": "basejump-unconcentrate-static-v1",
                "kind": "combinational",
                "contract": "PATTERN-selected output positions receive consecutive input bits; unselected positions are zero in simulation",
            },
            "configs": [
                {"OUTPUT_ELEMS": 1, "PATTERN": 1, "INPUT_ELEMS": 1},
                {"OUTPUT_ELEMS": 4, "PATTERN": 15, "INPUT_ELEMS": 4},
                {"OUTPUT_ELEMS": 4, "PATTERN": 11, "INPUT_ELEMS": 3},
                {"OUTPUT_ELEMS": 5, "PATTERN": 31, "INPUT_ELEMS": 5},
                {"OUTPUT_ELEMS": 8, "PATTERN": 239, "INPUT_ELEMS": 7},
            ],
        },
        {
            "name": "basejump-counter-clear-up-saturating",
            "module": "pyc_runtime_basejump_counter_clear_up_saturating",
            "implementation": "bsg_counter_clear_up_saturating",
            "family": "control",
            "wrapper": "verilog/pyc_runtime_basejump_counter_clear_up_saturating.sv",
            "source_file": "bsg_misc/bsg_counter_clear_up_saturating.sv",
            "source_files": [f"{BASEJUMP_VENDOR}/bsg_counter_clear_up_saturating.sv", BASEJUMP_DEFINES],
            "parameters": [
                {"name": "MAX_VALUE", "source": "max_val_p", "default": 3},
                {"name": "INIT_VALUE", "source": "init_val_p", "default": 0},
            ],
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "reset", "direction": "input", "width": "1"},
                {"name": "clear", "direction": "input", "width": "1"},
                {"name": "up", "direction": "input", "width": "1"},
                {"name": "count", "direction": "output", "width": "COUNT_WIDTH"},
            ],
            "oracle": {
                "id": "basejump-counter-clear-up-saturating-v1",
                "kind": "cycle",
                "contract": "reset loads INIT_VALUE; clear has priority and clear+up starts at one; up increments until MAX_VALUE",
            },
            "configs": [
                {"MAX_VALUE": 1, "INIT_VALUE": 0},
                {"MAX_VALUE": 3, "INIT_VALUE": 1},
                {"MAX_VALUE": 7, "INIT_VALUE": 0},
                {"MAX_VALUE": 15, "INIT_VALUE": 4},
            ],
        },
    ]


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


def _entry(spec: dict[str, Any], functional: dict[str, Any], gate: dict[str, Any]) -> dict[str, Any]:
    files = [spec["wrapper"], *spec["source_files"]]
    missing = [item for item in [*files, BASEJUMP_LICENSE] if not (ROOT / item).is_file()]
    functional_ok = functional.get("summary", {}).get("status") == "passed"
    gate_ok = gate.get("summary", {}).get("status") == "passed"
    qos: list[dict[str, Any]] = []
    for result in functional.get("results", []):
        for case in result.get("cases", []) if isinstance(result, dict) else []:
            if isinstance(case, dict):
                qor = case.get("qor", {})
                qos.append({"parameters": case.get("parameters", {}), **{key: qor.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    fpath = REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v12-{spec['name']}.json"
    gpath = REPO / ".pycircuit_out" / "runtime-catalog-validation" / f"v12-{spec['name']}.json"
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": "basejump-stl-v0.12", "provider": "github",
        "status": "accepted" if functional_ok and gate_ok and not missing else "pending",
        "family": spec["family"], "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "source_file": spec["source_file"], "license": "Solderpad-Hardware-License-0.51"},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": [BASEJUMP_LICENSE], "include_roots": []},
        "validation": {
            "status": "passed" if functional_ok and gate_ok and not missing else "pending",
            "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1",
            "manifest": "manifests/parameterized-components-v0.12.json", "configs": spec["configs"],
            "functional_report": _relative(fpath), "runtime_gate_report": _relative(gpath),
            "runtime_gate": "passed" if gate_ok else "pending", "qor": qos,
        },
    }


def main() -> int:
    catalog = _read(CATALOG)
    specs = normalize_specs(_specs())
    names = {spec["name"] for spec in specs}
    entries = [entry for entry in catalog.get("entries", []) if entry.get("name") not in names]
    for spec in specs:
        entries.append(_entry(spec,
                              _read(REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v12-{spec['name']}.json"),
                              _read(REPO / ".pycircuit_out" / "runtime-catalog-validation" / f"v12-{spec['name']}.json")))
    entries.sort(key=lambda entry: entry.get("name", ""))
    catalog["entries"] = entries
    catalog["runtime_api_version"] = "0.12"
    catalog["generated_by"] = "acir-runtime-promote-v12"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old_manifest = _read(ROOT / "manifests/parameterized-components-v0.11.json")
    components = [component for component in old_manifest.get("components", []) if component.get("name") not in names]
    for spec in specs:
        components.append({"name": spec["name"], "oracle": spec["oracle"],
                           "parameters": {item["name"]: item.get("source", "") for item in spec["parameters"]},
                           "configs": spec["configs"], "source": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "files": spec["source_files"], "license": BASEJUMP_LICENSE}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for item in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / item
            if path.is_file():
                hashes[item] = _sha(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.12", "release": "runtime-rtl-v0.12", "generated_by": "acir-runtime-promote-v12", "toolchain": old_manifest.get("toolchain", {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}), "policy": old_manifest.get("policy", {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed"}), "components": sorted(components, key=lambda component: component.get("name", "")), "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    lock = _read(LOCK)
    lock.setdefault("sources", {})["basejump-stl-v0.12"] = {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "license": "Solderpad-Hardware-License-0.51"}
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog_entries": len(entries), "new_components": len(specs), "accepted": sum(entry.get("status") == "accepted" for entry in entries if entry.get("name") in names)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
