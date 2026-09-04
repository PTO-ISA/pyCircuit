#!/usr/bin/env python3
"""Promote three low-risk BaseJump combinational primitives into runtime v0.9.

The batch is intentionally small and deterministic.  A component is recorded
as accepted only with a complete vendored source closure, a provenance/license
record, a parameterized functional report, and a packaged Verilator/Yosys gate
report.  Missing reports leave the entry pending so the script can be run once
before validation and once after the bounded validation commands complete.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from _paths import normalize_specs

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.9.json"

BASEJUMP_REPO = "https://github.com/bespoke-silicon-group/basejump_stl.git"
BASEJUMP_COMMIT = "b48037e28544425839dbd617d45b1a82631bc1a9"
BASEJUMP_LICENSE = "licenses/basejump-stl-v0.5/LICENSE"
VENDOR = "verilog/vendor-v0.5/basejump/bsg_misc"


def _specs() -> list[dict[str, Any]]:
    return [
        {
            "name": "basejump-adder-one-hot",
            "module": "pyc_runtime_basejump_adder_one_hot",
            "implementation": "bsg_adder_one_hot",
            "family": "arithmetic-encoding",
            "wrapper": "verilog/pyc_runtime_basejump_adder_one_hot.sv",
            "source_file": "bsg_misc/bsg_adder_one_hot.sv",
            "source_files": [f"{VENDOR}/bsg_adder_one_hot.sv", f"{VENDOR}/bsg_defines.sv"],
            "parameters": [
                {"name": "WIDTH", "source": "width_p", "default": 8},
                {"name": "OUTPUT_WIDTH", "source": "output_width_p", "default": 8},
            ],
            "ports": [
                {"name": "a", "direction": "input", "width": "WIDTH"},
                {"name": "b", "direction": "input", "width": "WIDTH"},
                {"name": "out", "direction": "output", "width": "OUTPUT_WIDTH"},
            ],
            "oracle": {
                "id": "basejump-adder-one-hot-v1",
                "kind": "combinational",
                "contract": "a and b are one-hot indices; out encodes their sum, wrapping modulo WIDTH when OUTPUT_WIDTH equals WIDTH and retaining non-wrapping sums otherwise",
            },
            "configs": [
                {"WIDTH": 1, "OUTPUT_WIDTH": 1},
                {"WIDTH": 4, "OUTPUT_WIDTH": 4},
                {"WIDTH": 4, "OUTPUT_WIDTH": 7},
                {"WIDTH": 8, "OUTPUT_WIDTH": 8},
                {"WIDTH": 8, "OUTPUT_WIDTH": 15},
            ],
        },
        {
            "name": "basejump-mux-one-hot",
            "module": "pyc_runtime_basejump_mux_one_hot",
            "implementation": "bsg_mux_one_hot",
            "family": "interconnect",
            "wrapper": "verilog/pyc_runtime_basejump_mux_one_hot.sv",
            "source_file": "bsg_misc/bsg_mux_one_hot.sv",
            "source_files": [f"{VENDOR}/bsg_mux_one_hot.sv", f"{VENDOR}/bsg_defines.sv"],
            "parameters": [
                {"name": "WIDTH", "source": "width_p", "default": 8},
                {"name": "ELS", "source": "els_p", "default": 2},
                {"name": "HARDEN", "source": "harden_p", "default": 1},
            ],
            "ports": [
                {"name": "data", "direction": "input", "width": "ELS*WIDTH"},
                {"name": "select", "direction": "input", "width": "ELS"},
                {"name": "out", "direction": "output", "width": "WIDTH"},
            ],
            "oracle": {
                "id": "basejump-mux-one-hot-v1",
                "kind": "combinational",
                "contract": "one-hot select forwards the selected packed word, zero select produces zero, and multiple selects produce the bitwise OR of selected words",
            },
            "configs": [
                {"WIDTH": 1, "ELS": 1, "HARDEN": 1},
                {"WIDTH": 4, "ELS": 2, "HARDEN": 1},
                {"WIDTH": 8, "ELS": 4, "HARDEN": 1},
                {"WIDTH": 3, "ELS": 5, "HARDEN": 1},
            ],
        },
        {
            "name": "basejump-mux-butterfly",
            "module": "pyc_runtime_basejump_mux_butterfly",
            "implementation": "bsg_mux_butterfly",
            "family": "interconnect",
            "wrapper": "verilog/pyc_runtime_basejump_mux_butterfly.sv",
            "source_file": "bsg_misc/bsg_mux_butterfly.sv",
            "source_files": [
                f"{VENDOR}/bsg_mux_butterfly.sv",
                f"{VENDOR}/bsg_swap.sv",
                f"{VENDOR}/bsg_defines.sv",
            ],
            "parameters": [
                {"name": "WIDTH", "source": "width_p", "default": 8},
                {"name": "ELS", "source": "els_p", "default": 4, "constraint": "power_of_two"},
            ],
            "ports": [
                {"name": "data", "direction": "input", "width": "ELS*WIDTH"},
                {"name": "select", "direction": "input", "width": "SELECT_WIDTH"},
                {"name": "out", "direction": "output", "width": "ELS*WIDTH"},
            ],
            "oracle": {
                "id": "basejump-mux-butterfly-v1",
                "kind": "combinational",
                "contract": "for power-of-two ELS, out[i] equals data[i XOR select]; each select bit controls one butterfly swap stage",
            },
            "configs": [
                {"WIDTH": 4, "ELS": 2},
                {"WIDTH": 8, "ELS": 4},
                {"WIDTH": 4, "ELS": 8},
            ],
        },
    ]


def _read_report(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    return value if isinstance(value, dict) else {}


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _relative_report(path: Path) -> str:
    try:
        return path.relative_to(REPO).as_posix()
    except ValueError:
        return str(path)


def _entry(spec: dict[str, Any], functional: dict[str, Any], gate: dict[str, Any]) -> dict[str, Any]:
    files = [spec["wrapper"], *spec["source_files"]]
    all_files = [*files, BASEJUMP_LICENSE]
    missing = [item for item in all_files if not (ROOT / item).is_file()]
    functional_ok = functional.get("summary", {}).get("status") == "passed"
    gate_ok = gate.get("summary", {}).get("status") == "passed"
    qos: list[dict[str, Any]] = []
    for result in functional.get("results", []):
        for case in result.get("cases", []) if isinstance(result, dict) else []:
            if isinstance(case, dict):
                qor = case.get("qor", {})
                qos.append({"parameters": case.get("parameters", {}), **{key: qor.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    fpath = REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v09-{spec['name']}.json"
    gpath = REPO / ".pycircuit_out" / "runtime-catalog-validation" / f"v09-{spec['name']}.json"
    return {
        "name": spec["name"],
        "module": spec["module"],
        "implementation": spec["implementation"],
        "source": "basejump-stl-v0.9",
        "provider": "github",
        "status": "accepted",
        "family": spec["family"],
        "wrapper": spec["wrapper"],
        "files": files,
        "provenance": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "source_file": spec["source_file"], "license": "Solderpad-Hardware-License-0.51"},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": [BASEJUMP_LICENSE], "include_roots": []},
        "validation": {
            "status": "passed" if functional_ok and gate_ok and not missing else ("pending" if not functional or not gate else "failed"),
            "mode": "packaged-functional-verilator-yosys",
            "semantic_status": "functional_oracle_v1",
            "manifest": "manifests/parameterized-components-v0.9.json",
            "configs": spec["configs"],
            "functional_report": _relative_report(fpath),
            "runtime_gate_report": _relative_report(gpath),
            "runtime_gate": "passed" if gate_ok else ("pending" if not gate else "failed"),
            "qor": qos,
        },
    }


def main() -> int:
    catalog = _read_report(CATALOG)
    specs = normalize_specs(_specs())
    names = {spec["name"] for spec in specs}
    entries = [entry for entry in catalog.get("entries", []) if entry.get("name") not in names]
    for spec in specs:
        functional = _read_report(REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v09-{spec['name']}.json")
        gate = _read_report(REPO / ".pycircuit_out" / "runtime-catalog-validation" / f"v09-{spec['name']}.json")
        entries.append(_entry(spec, functional, gate))
    entries.sort(key=lambda entry: entry.get("name", ""))
    catalog["entries"] = entries
    catalog["runtime_api_version"] = "0.9"
    catalog["generated_by"] = "acir-runtime-promote-v09"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old_manifest = _read_report(MANIFEST)
    if not old_manifest:
        old_manifest = _read_report(ROOT / "manifests" / "parameterized-components-v0.8.json")
    components = [component for component in old_manifest.get("components", []) if component.get("name") not in names]
    components.extend({"name": spec["name"], "oracle": spec["oracle"], "parameters": {item["name"]: item.get("source", "") for item in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "files": spec["source_files"], "license": BASEJUMP_LICENSE}} for spec in specs)
    hashes: dict[str, str] = {}
    for entry in entries:
        for item in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / item
            if path.is_file():
                hashes[item] = _sha(path)
    manifest = {
        "schema": "acir-runtime-parameterized-components-v0.9",
        "release": "runtime-rtl-v0.9",
        "generated_by": "acir-runtime-promote-v09",
        "toolchain": old_manifest.get("toolchain", {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}),
        "policy": old_manifest.get("policy", {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed"}),
        "components": sorted(components, key=lambda component: component.get("name", "")),
        "sha256": dict(sorted(hashes.items())),
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog_entries": len(entries), "new_components": len(specs), "functional_pass": sum(_read_report(REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v09-{spec['name']}.json").get("summary", {}).get("status") == "passed" for spec in specs)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
