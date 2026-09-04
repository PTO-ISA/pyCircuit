#!/usr/bin/env python3
"""Promote the next bounded BaseJump/PULP runtime batch (v0.10)."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from _paths import normalize_specs

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.10.json"

BASEJUMP_REPO = "https://github.com/bespoke-silicon-group/basejump_stl.git"
BASEJUMP_COMMIT = "b48037e28544425839dbd617d45b1a82631bc1a9"
BASEJUMP_LICENSE = "licenses/basejump-stl-v0.5/LICENSE"
BASEJUMP_VENDOR = "verilog/vendor-v0.5/basejump/bsg_misc"
PULP_REPO = "https://github.com/pulp-platform/common_cells.git"
PULP_COMMIT = "db42769334b4589b4b3fc671b34513bdb98be565"
PULP_LICENSE = "licenses/pulp-common-cells-v0.10-LICENSE"
PULP_VENDOR = "verilog/vendor-v0.10/pulp_common_cells"


def _specs() -> list[dict[str, Any]]:
    return [
        {
            "name": "basejump-array-concentrate-static",
            "module": "pyc_runtime_basejump_array_concentrate_static",
            "implementation": "bsg_array_concentrate_static",
            "source": "basejump-stl-v0.10",
            "family": "interconnect",
            "wrapper": "verilog/pyc_runtime_basejump_array_concentrate_static.sv",
            "source_file": "bsg_misc/bsg_array_concentrate_static.sv",
            "source_files": [f"{BASEJUMP_VENDOR}/bsg_array_concentrate_static.sv", f"{BASEJUMP_VENDOR}/bsg_defines.sv"],
            "license": BASEJUMP_LICENSE,
            "provenance_license": "Solderpad-Hardware-License-0.51",
            "parameters": [
                {"name": "WIDTH", "source": "width_p", "default": 8},
                {"name": "DENSE_ELEMS", "source": "pattern_els_p width", "default": 4},
                {"name": "PATTERN", "source": "pattern_els_p", "default": 15},
            ],
            "ports": [
                {"name": "data", "direction": "input", "width": "DENSE_ELEMS*WIDTH"},
                {"name": "out", "direction": "output", "width": "SPARSE_ELEMS*WIDTH"},
            ],
            "oracle": {"id": "basejump-array-concentrate-static-v1", "kind": "combinational", "contract": "selected PATTERN bits are copied in ascending source-index order into a densely packed output array"},
            "configs": [
                {"WIDTH": 1, "DENSE_ELEMS": 2, "PATTERN": 3, "SPARSE_ELEMS": 2},
                {"WIDTH": 4, "DENSE_ELEMS": 4, "PATTERN": 15, "SPARSE_ELEMS": 4},
                {"WIDTH": 8, "DENSE_ELEMS": 4, "PATTERN": 11, "SPARSE_ELEMS": 3},
                {"WIDTH": 3, "DENSE_ELEMS": 5, "PATTERN": 31, "SPARSE_ELEMS": 5},
                {"WIDTH": 4, "DENSE_ELEMS": 8, "PATTERN": 239, "SPARSE_ELEMS": 7},
            ],
        },
        {
            "name": "pulp-credit-counter",
            "module": "pyc_runtime_pulp_credit_counter",
            "implementation": "cc_credit_counter",
            "source": "pulp-common-cells-v0.10",
            "family": "control",
            "wrapper": "verilog/pyc_runtime_pulp_credit_counter.sv",
            "source_file": "src/cc_credit_counter.sv",
            "source_files": [
                f"{PULP_VENDOR}/src/cc_credit_counter.sv",
                f"{PULP_VENDOR}/include/common_cells/registers.svh",
                f"{PULP_VENDOR}/include/common_cells/deprecated/registers.svh",
                f"{PULP_VENDOR}/include/common_cells/assertions.svh",
            ],
            "license": PULP_LICENSE,
            "provenance_license": "Solderpad-Hardware-License-0.51",
            "parameters": [
                {"name": "NUM_CREDITS", "source": "NumCredits", "default": 4},
                {"name": "INIT_EMPTY", "source": "InitCreditEmpty", "default": 0},
            ],
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "rst_n", "direction": "input", "width": "1"},
                {"name": "clear", "direction": "input", "width": "1"},
                {"name": "credit", "direction": "output", "width": "CREDIT_WIDTH"},
                {"name": "give", "direction": "input", "width": "1"},
                {"name": "take", "direction": "input", "width": "1"},
                {"name": "credit_left", "direction": "output", "width": "1"},
                {"name": "credit_critical", "direction": "output", "width": "1"},
                {"name": "credit_full", "direction": "output", "width": "1"},
            ],
            "oracle": {"id": "pulp-credit-counter-v1", "kind": "cycle", "contract": "asynchronous active-low reset and synchronous clear initialize credit; give increments, take decrements, simultaneous give/take holds, and status flags reflect the count"},
            "configs": [
                {"NUM_CREDITS": 1, "INIT_EMPTY": 0},
                {"NUM_CREDITS": 2, "INIT_EMPTY": 1},
                {"NUM_CREDITS": 4, "INIT_EMPTY": 0},
                {"NUM_CREDITS": 7, "INIT_EMPTY": 1},
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
    missing = [item for item in [*files, spec["license"]] if not (ROOT / item).is_file()]
    functional_ok = functional.get("summary", {}).get("status") == "passed"
    gate_ok = gate.get("summary", {}).get("status") == "passed"
    qos: list[dict[str, Any]] = []
    for result in functional.get("results", []):
        for case in result.get("cases", []) if isinstance(result, dict) else []:
            if isinstance(case, dict):
                qor = case.get("qor", {})
                qos.append({"parameters": case.get("parameters", {}), **{key: qor.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    fpath = REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v10-{spec['name']}.json"
    gpath = REPO / ".pycircuit_out" / "runtime-catalog-validation" / f"v10-{spec['name']}.json"
    include_roots = [f"{PULP_VENDOR}/include"] if spec["name"].startswith("pulp-") else []
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": spec["source"], "provider": "github", "status": "accepted", "family": spec["family"],
        "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": PULP_REPO if spec["name"].startswith("pulp-") else BASEJUMP_REPO, "commit": PULP_COMMIT if spec["name"].startswith("pulp-") else BASEJUMP_COMMIT, "source_file": spec["source_file"], "license": spec["provenance_license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": [spec["license"]], "include_roots": include_roots},
        "validation": {"status": "passed" if functional_ok and gate_ok and not missing else ("pending" if not functional or not gate else "failed"), "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.10.json", "configs": spec["configs"], "functional_report": _relative(fpath), "runtime_gate_report": _relative(gpath), "runtime_gate": "passed" if gate_ok else ("pending" if not gate else "failed"), "qor": qos},
    }


def main() -> int:
    catalog = _read(CATALOG)
    specs = normalize_specs(_specs())
    names = {spec["name"] for spec in specs}
    entries = [entry for entry in catalog.get("entries", []) if entry.get("name") not in names]
    for spec in specs:
        entries.append(_entry(spec, _read(REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v10-{spec['name']}.json"), _read(REPO / ".pycircuit_out" / "runtime-catalog-validation" / f"v10-{spec['name']}.json")))
    entries.sort(key=lambda entry: entry.get("name", ""))
    catalog["entries"] = entries
    catalog["runtime_api_version"] = "0.10"
    catalog["generated_by"] = "acir-runtime-promote-v10"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old_manifest = _read(MANIFEST) or _read(ROOT / "manifests/parameterized-components-v0.9.json")
    components = [component for component in old_manifest.get("components", []) if component.get("name") not in names]
    components.extend({"name": spec["name"], "oracle": spec["oracle"], "parameters": {item["name"]: item.get("source", "") for item in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": PULP_REPO if spec["name"].startswith("pulp-") else BASEJUMP_REPO, "commit": PULP_COMMIT if spec["name"].startswith("pulp-") else BASEJUMP_COMMIT, "files": spec["source_files"], "license": spec["license"]}} for spec in specs)
    hashes: dict[str, str] = {}
    for entry in entries:
        for item in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / item
            if path.is_file():
                hashes[item] = _sha(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.10", "release": "runtime-rtl-v0.10", "generated_by": "acir-runtime-promote-v10", "toolchain": old_manifest.get("toolchain", {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}), "policy": old_manifest.get("policy", {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed"}), "components": sorted(components, key=lambda component: component.get("name", "")), "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog_entries": len(entries), "new_components": len(specs), "functional_pass": sum(_read(REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v10-{spec['name']}.json").get("summary", {}).get("status") == "passed" for spec in specs)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
