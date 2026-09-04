#!/usr/bin/env python3
"""Promote two BaseJump round-robin variants as runtime RTL v0.21."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Mapping

from _paths import normalize_specs

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.21.json"
SOURCE_FILES = [
    "verilog/vendor-v0.5/basejump/bsg_misc/bsg_arb_round_robin.sv",
    "verilog/vendor-v0.5/basejump/bsg_misc/bsg_scan.sv",
    "verilog/vendor-v0.5/basejump/bsg_misc/bsg_defines.sv",
]
LICENSE_FILE = "licenses/basejump-stl-v0.5/LICENSE"
REPOSITORY = "https://github.com/bespoke-silicon-group/basejump_stl.git"
COMMIT = "b48037e28544425839dbd617d45b1a82631bc1a9"


def _specs() -> list[dict[str, Any]]:
    common = {
        "source": "basejump-stl-v0.5",
        "provider": "github",
        "family": "arbitration-interconnect",
        "source_files": SOURCE_FILES,
        "repository": REPOSITORY,
        "commit": COMMIT,
        "source_file": "bsg_misc/bsg_arb_round_robin.sv",
        "license": "Solderpad-Hardware-License-0.51",
        "license_file": LICENSE_FILE,
        "parameters": [{"name": "NUM_INPUTS", "source": "width_p", "default": 4}],
        "configs": [{"NUM_INPUTS": 2}, {"NUM_INPUTS": 4}, {"NUM_INPUTS": 8}],
    }
    return [
        {
            **common,
            "name": "basejump-rr-composable",
            "module": "pyc_runtime_basejump_rr_composable",
            "implementation": "bsg_arb_round_robin_composable",
            "wrapper": "verilog/pyc_runtime_basejump_rr_composable.sv",
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "reset", "direction": "input", "width": "1"},
                {"name": "requests", "direction": "input", "width": "NUM_INPUTS"},
                {"name": "thermocode", "direction": "input", "width": "max(1, NUM_INPUTS-1)"},
                {"name": "grant", "direction": "output", "width": "NUM_INPUTS"},
                {"name": "grant_valid", "direction": "output", "width": "1"},
                {"name": "thermocode_next", "direction": "output", "width": "max(1, NUM_INPUTS-1)"},
            ],
            "oracle": {
                "id": "basejump-rr-composable-v1",
                "kind": "combinational/state-externalized",
                "contract": "given an external thermometer pointer, emit one high-to-low cyclic one-hot grant and the corresponding next pointer",
            },
        },
        {
            **common,
            "name": "basejump-rr-two-level",
            "module": "pyc_runtime_basejump_rr_two_level",
            "implementation": "bsg_arb_round_robin_two_level",
            "wrapper": "verilog/pyc_runtime_basejump_rr_two_level.sv",
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "reset", "direction": "input", "width": "1"},
                {"name": "requests_high_low", "direction": "input", "width": "2*NUM_INPUTS"},
                {"name": "advance", "direction": "input", "width": "1"},
                {"name": "grant", "direction": "output", "width": "NUM_INPUTS"},
                {"name": "grant_valid", "direction": "output", "width": "1"},
                {"name": "granted_high", "direction": "output", "width": "1"},
            ],
            "oracle": {
                "id": "basejump-rr-two-level-v1",
                "kind": "cycle/arbitration",
                "contract": "high-priority requests always precede low-priority requests while each plane advances round-robin state only on acceptance",
            },
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


def _result(report: Mapping[str, Any], name: str) -> dict[str, Any]:
    return next((dict(item) for item in report.get("results", [])
                 if isinstance(item, Mapping) and str(item.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(item) for item in spec["source_files"]]]
    licenses = [str(spec["license_file"])]
    missing = [path for path in [*files, *licenses] if not (ROOT / path).is_file()]
    f = _result(functional, str(spec["name"])); g = _result(gate, str(spec["name"]))
    staging = not functional and not gate
    accepted = not missing and (staging or (f.get("status") == "passed" and g.get("status") == "passed"))
    qor = []
    for case in f.get("cases", []) if isinstance(f, Mapping) else []:
        if isinstance(case, Mapping) and isinstance(case.get("qor"), Mapping):
            q = case["qor"]
            qor.append({"parameters": case.get("parameters", {}),
                        **{key: q.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": spec["source"], "provider": spec["provider"], "status": "accepted" if accepted else "pending",
        "family": spec["family"], "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": spec["repository"], "commit": spec["commit"],
                       "source_file": spec["source_file"], "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete",
                               "source_files": spec["source_files"], "license_files": licenses,
                               "include_roots": ["verilog/vendor-v0.5/basejump/bsg_misc"]},
        "validation": {
            "status": "passed" if accepted and not staging else "pending",
            "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1",
            "manifest": "manifests/parameterized-components-v0.21.json", "configs": spec["configs"], "qor": qor,
            "functional_report": ".pycircuit_out/runtime-functional-validation/v21-basejump-round-robin.json",
            "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v21-basejump-round-robin.json",
            "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"),
        },
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending",
                         "top": spec["module"], "files": files,
                         "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path,
                        default=REPO / ".pycircuit_out/runtime-functional-validation/v21-basejump-round-robin.json")
    parser.add_argument("--gate-report", type=Path,
                        default=REPO / ".pycircuit_out/runtime-catalog-validation/v21-basejump-round-robin.json")
    args = parser.parse_args()
    functional = _read(args.functional_report) if args.mode == "finalize" else {}
    gate = _read(args.gate_report) if args.mode == "finalize" else {}
    specs = normalize_specs(_specs()); names = {str(spec["name"]) for spec in specs}
    catalog = _read(CATALOG)
    entries = [entry for entry in catalog.get("entries", []) if str(entry.get("name")) not in names]
    entries.extend(_entry(spec, functional, gate) for spec in specs)
    entries.sort(key=lambda entry: str(entry.get("name", "")))
    catalog["entries"] = entries; catalog["runtime_api_version"] = "0.21"; catalog["generated_by"] = "acir-runtime-promote-v21"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old = _read(ROOT / "manifests/parameterized-components-v0.20.json")
    components = [component for component in old.get("components", []) if str(component.get("name")) not in names]
    for spec in specs:
        components.append({"name": spec["name"], "oracle": spec["oracle"],
                           "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]},
                           "configs": spec["configs"],
                           "source": {"repository": spec["repository"], "commit": spec["commit"],
                                      "files": spec["source_files"], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for path_text in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(path_text)
            if path.is_file():
                hashes[str(path_text)] = _sha(path)
    MANIFEST.write_text(json.dumps({
        "schema": "acir-runtime-parameterized-components-v0.21", "release": "runtime-rtl-v0.21",
        "generated_by": "acir-runtime-promote-v21", "toolchain": old.get("toolchain", {}),
        "policy": old.get("policy", {}),
        "components": sorted(components, key=lambda component: str(component.get("name", ""))),
        "sha256": dict(sorted(hashes.items())),
    }, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    promoted = [entry for entry in entries if str(entry.get("name")) in names]
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": len(specs),
                      "accepted": sum(entry.get("status") == "accepted" for entry in promoted)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
