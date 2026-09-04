#!/usr/bin/env python3
"""Promote OpenTitan Hamming(76,68) primitives into runtime v0.15.

The promotion is report-driven: ``prepare`` registers the reviewed wrappers
so the functional runner can exercise them, while ``finalize`` records the
Verilator/Yosys evidence and emits a self-contained release manifest.
"""

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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.15.json"
LOCK = ROOT / "catalog.lock.json"

OPEN_TITAN_REPO = "https://github.com/lowRISC/opentitan.git"
OPEN_TITAN_COMMIT = "b16f2be75d2f38c62d861208453ed5b81ccf41b0"
LICENSE_FILE = "licenses/opentitan-v0.14-LICENSE"
VENDOR = "verilog/vendor-v0.15/opentitan/hw/ip/prim/rtl"


def _specs() -> list[dict[str, Any]]:
    common = {
        "source": "opentitan-v0.15",
        "provider": "github",
        "family": "ecc",
        "repository": OPEN_TITAN_REPO,
        "commit": OPEN_TITAN_COMMIT,
        "license": "Apache-2.0",
        "license_file": LICENSE_FILE,
    }
    rows = [
        ("hamming", "enc", "prim_secded_hamming_76_68_enc", "68", "76"),
        ("hamming", "dec", "prim_secded_hamming_76_68_dec", "76", "68"),
        ("inv_hamming", "enc", "prim_secded_inv_hamming_76_68_enc", "68", "76"),
        ("inv_hamming", "dec", "prim_secded_inv_hamming_76_68_dec", "76", "68"),
    ]
    specs: list[dict[str, Any]] = []
    for variant, direction, implementation, input_width, output_width in rows:
        suffix = implementation.removeprefix("prim_")
        name = f"opentitan-{suffix.replace('_', '-')}"
        module = f"pyc_runtime_opentitan_{suffix}"
        wrapper = f"verilog/{module}.sv"
        source_file = f"{VENDOR}/{implementation}.sv"
        if direction == "enc":
            ports = [
                {"name": "data_in", "direction": "input", "width": input_width},
                {"name": "data_out", "direction": "output", "width": output_width},
            ]
            contract = f"data_out is the OpenTitan {variant.replace('_', ' ')} SECDED codeword for 68-bit data"
        else:
            ports = [
                {"name": "data_in", "direction": "input", "width": input_width},
                {"name": "data_out", "direction": "output", "width": output_width},
                {"name": "syndrome", "direction": "output", "width": "8"},
                {"name": "error", "direction": "output", "width": "2"},
            ]
            contract = "clean codewords decode to the original data and single-bit errors are corrected/reported"
        specs.append({
            **common,
            "name": name,
            "module": module,
            "implementation": implementation,
            "wrapper": wrapper,
            "source_file": f"hw/ip/prim/rtl/{implementation}.sv",
            "source_files": [source_file],
            "parameters": [],
            "ports": ports,
            "oracle": {"id": f"{name}-v1", "kind": "combinational", "contract": contract},
            "configs": [{}],
        })
    return specs


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
    for item in report.get("results", []):
        if isinstance(item, Mapping) and str(item.get("name")) == name:
            return dict(item)
    return {}


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(item) for item in spec["source_files"]]]
    missing = [item for item in [*files, str(spec["license_file"])] if not (ROOT / item).is_file()]
    fitem = _result(functional, str(spec["name"]))
    gitem = _result(gate, str(spec["name"]))
    staging = not functional and not gate
    accepted = not missing and (staging or (fitem.get("status") == "passed" and gitem.get("status") == "passed"))
    qos: list[dict[str, Any]] = []
    for case in fitem.get("cases", []) if isinstance(fitem, Mapping) else []:
        if isinstance(case, Mapping) and isinstance(case.get("qor"), Mapping):
            qor = case["qor"]
            qos.append({"parameters": case.get("parameters", {}), **{key: qor.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    fpath = REPO / ".pycircuit_out" / "runtime-functional-validation" / "v15-opentitan-hamming-76-68.json"
    gpath = REPO / ".pycircuit_out" / "runtime-catalog-validation" / "v15-opentitan-hamming-76-68.json"
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": spec["source"], "provider": spec["provider"], "status": "accepted" if accepted else "pending",
        "family": spec["family"], "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": spec["source_file"], "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": [spec["license_file"]], "include_roots": []},
        "validation": {"status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.15.json", "configs": spec["configs"], "functional_report": _relative(fpath), "runtime_gate_report": _relative(gpath), "runtime_gate": "passed" if gitem.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qos},
        "verification": {"name": spec["name"], "status": "passed" if gitem.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": gitem.get("verilator", {}), "yosys": gitem.get("yosys", {})},
    }


def _write_manifest(entries: list[dict[str, Any]], specs: list[dict[str, Any]]) -> None:
    old = _read(ROOT / "manifests/parameterized-components-v0.14.json")
    names = {str(spec["name"]) for spec in specs}
    components = [item for item in old.get("components", []) if str(item.get("name")) not in names]
    for spec in specs:
        components.append({"name": spec["name"], "oracle": spec["oracle"], "parameters": {}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["source_files"], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for item in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(item)
            if path.is_file():
                hashes[str(item)] = _sha(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.15", "release": "runtime-rtl-v0.15", "generated_by": "acir-runtime-promote-v15", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda item: str(item.get("name", ""))), "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v15-opentitan-hamming-76-68.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v15-opentitan-hamming-76-68.json")
    args = parser.parse_args()
    specs = normalize_specs(_specs())
    catalog = _read(CATALOG)
    names = {str(spec["name"]) for spec in specs}
    functional = _read(args.functional_report) if args.mode == "finalize" else {}
    gate = _read(args.gate_report) if args.mode == "finalize" else {}
    entries = [entry for entry in catalog.get("entries", []) if str(entry.get("name")) not in names]
    entries.extend(_entry(spec, functional, gate) for spec in specs)
    entries.sort(key=lambda item: str(item.get("name", "")))
    catalog["entries"] = entries
    catalog["runtime_api_version"] = "0.15"
    catalog["generated_by"] = "acir-runtime-promote-v15"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    _write_manifest(entries, specs)
    lock = _read(LOCK)
    lock.setdefault("sources", {})["opentitan-v0.15"] = {"repository": OPEN_TITAN_REPO, "commit": OPEN_TITAN_COMMIT, "license": "Apache-2.0"}
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    accepted = sum(entry.get("status") == "accepted" for entry in entries if entry.get("name") in names)
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": len(specs), "accepted": accepted}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
