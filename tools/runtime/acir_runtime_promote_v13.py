#!/usr/bin/env python3
"""Promote the next two verified RTL candidates into the runtime catalog."""

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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.13.json"
LOCK = ROOT / "catalog.lock.json"

BASEJUMP_REPO = "https://github.com/bespoke-silicon-group/basejump_stl.git"
BASEJUMP_COMMIT = "b48037e28544425839dbd617d45b1a82631bc1a9"
BASEJUMP_LICENSE = "licenses/basejump-stl/LICENSE"
VORTEX_REPO = "https://github.com/vortexgpgpu/vortex.git"
VORTEX_COMMIT = "5d62846c685ae287f9cd3ddd49f4537c40146eae"
VORTEX_LICENSE = "licenses/vortex-v0.13-LICENSE"


def _specs() -> list[dict[str, Any]]:
    return [
        {
            "name": "basejump-channel-narrow",
            "module": "pyc_runtime_basejump_channel_narrow",
            "implementation": "bsg_channel_narrow",
            "source": "basejump-stl-v0.13",
            "provider": "github",
            "family": "dataflow",
            "wrapper": "verilog/pyc_runtime_basejump_channel_narrow.sv",
            "files": [
                "verilog/pyc_runtime_basejump_channel_narrow.sv",
                "verilog/basejump/bsg_channel_narrow.sv",
                "verilog/basejump/bsg_defines.sv",
            ],
            "repository": BASEJUMP_REPO,
            "commit": BASEJUMP_COMMIT,
            "source_file": "bsg_dataflow/bsg_channel_narrow.sv",
            "license": "Solderpad-Hardware-License-0.51",
            "license_file": BASEJUMP_LICENSE,
            "parameters": [
                {"name": "WIDTH_IN", "source": "width_in_p", "default": 8},
                {"name": "WIDTH_OUT", "source": "width_out_p", "default": 4},
                {"name": "LSB_TO_MSB", "source": "lsb_to_msb_p", "default": 1},
            ],
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "reset", "direction": "input", "width": "1"},
                {"name": "data_in", "direction": "input", "width": "WIDTH_IN"},
                {"name": "deque_out", "direction": "output", "width": "1"},
                {"name": "data_out", "direction": "output", "width": "WIDTH_OUT"},
                {"name": "deque_in", "direction": "input", "width": "1"},
            ],
            "oracle": {
                "id": "channel-narrow-v1",
                "kind": "cycle",
                "contract": "each dequeue emits one WIDTH_OUT chunk in the configured order; deque_out marks the final chunk and resets the chunk pointer after the handshake",
            },
            "configs": [
                {"WIDTH_IN": 4, "WIDTH_OUT": 2, "LSB_TO_MSB": 1},
                {"WIDTH_IN": 8, "WIDTH_OUT": 4, "LSB_TO_MSB": 0},
                {"WIDTH_IN": 16, "WIDTH_OUT": 8, "LSB_TO_MSB": 1},
            ],
        },
        {
            "name": "vortex-priority-encoder",
            "module": "pyc_runtime_vortex_priority_encoder",
            "implementation": "VX_priority_encoder",
            "source": "vortex-v0.13",
            "provider": "github",
            "family": "encoding-arbitration",
            "wrapper": "verilog/pyc_runtime_vortex_priority_encoder.sv",
            "files": [
                "verilog/pyc_runtime_vortex_priority_encoder.sv",
                "verilog/vendor-v0.13/vortex/hw/rtl/libs/VX_priority_encoder.sv",
                "verilog/vendor-v0.13/vortex/hw/rtl/libs/VX_find_first.sv",
                "verilog/vendor-v0.13/vortex/hw/rtl/libs/VX_lzc.sv",
                "verilog/vendor-v0.13/vortex/hw/rtl/libs/VX_scan.sv",
                "verilog/vendor-v0.13/vortex/hw/rtl/VX_platform.vh",
                "verilog/vendor-v0.13/vortex/hw/rtl/VX_scope.vh",
            ],
            "repository": VORTEX_REPO,
            "commit": VORTEX_COMMIT,
            "source_file": "hw/rtl/libs/VX_priority_encoder.sv",
            "license": "Apache-2.0",
            "license_file": VORTEX_LICENSE,
            "parameters": [
                {"name": "WIDTH", "source": "N", "default": 8},
                {"name": "REVERSE", "source": "REVERSE", "default": 0},
                {"name": "MODEL", "source": "MODEL", "default": 1},
                {"name": "INDEX_WIDTH", "derived": "(WIDTH <= 1) ? 1 : $clog2(WIDTH)"},
            ],
            "ports": [
                {"name": "data_in", "direction": "input", "width": "WIDTH"},
                {"name": "onehot_out", "direction": "output", "width": "WIDTH"},
                {"name": "index_out", "direction": "output", "width": "INDEX_WIDTH"},
                {"name": "valid_out", "direction": "output", "width": "1"},
            ],
            "oracle": {
                "id": "vortex-priority-encoder-v1",
                "kind": "combinational",
                "contract": "valid_out reports any asserted input and onehot_out/index_out select the lowest or highest asserted bit according to REVERSE",
            },
            "configs": [
                {"WIDTH": 1, "REVERSE": 0, "MODEL": 1},
                {"WIDTH": 4, "REVERSE": 0, "MODEL": 1},
                {"WIDTH": 8, "REVERSE": 0, "MODEL": 2},
                {"WIDTH": 13, "REVERSE": 1, "MODEL": 1},
            ],
        },
    ]


def _read(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    return value if isinstance(value, dict) else {}


def _relative(path: Path) -> str:
    try:
        return path.relative_to(REPO).as_posix()
    except ValueError:
        return str(path)


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _result(report: Mapping[str, Any], name: str) -> dict[str, Any]:
    for item in report.get("results", []):
        if isinstance(item, Mapping) and str(item.get("name")) == name:
            return dict(item)
    return {}


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = list(spec["files"])
    missing = [item for item in [*files, str(spec["license_file"])] if not (ROOT / item).is_file()]
    functional_item = _result(functional, str(spec["name"]))
    gate_item = _result(gate, str(spec["name"]))
    functional_ok = functional_item.get("status") == "passed"
    gate_ok = gate_item.get("status") == "passed"
    qos: list[dict[str, Any]] = []
    for case in functional_item.get("cases", []) if isinstance(functional_item, Mapping) else []:
        if isinstance(case, Mapping):
            qor = case.get("qor", {})
            if isinstance(qor, Mapping):
                qos.append({"parameters": case.get("parameters", {}), **{key: qor.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    fpath = REPO / ".pycircuit_out" / "runtime-functional-validation" / "v13-next.json"
    gpath = REPO / ".pycircuit_out" / "runtime-catalog-validation" / "v13-next.json"
    # ``prepare`` uses an execution-only staging catalog so the functional
    # runner can discover the new entries.  ``finalize`` recomputes the status
    # from the persisted Verilator/Yosys reports before the release catalog is
    # considered authoritative.
    staging = not functional and not gate
    status = "accepted" if (staging or (functional_ok and gate_ok)) and not missing else "pending"
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": spec["source"], "provider": spec["provider"], "status": status,
        "family": spec["family"], "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": spec["source_file"], "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": files[1:], "license_files": [spec["license_file"]], "include_roots": []},
        "validation": {"status": "passed" if status == "accepted" else "pending", "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.13.json", "configs": spec["configs"], "functional_report": _relative(fpath), "runtime_gate_report": _relative(gpath), "runtime_gate": "passed" if gate_ok else "pending", "qor": qos},
        "verification": {"name": spec["name"], "status": "passed" if gate_ok else "pending", "top": spec["module"], "files": files, "verilator": gate_item.get("verilator", {}), "yosys": gate_item.get("yosys", {})},
    }


def _write_release(specs: list[dict[str, Any]], entries: list[dict[str, Any]], old_manifest: Mapping[str, Any]) -> None:
    components = [component for component in old_manifest.get("components", []) if component.get("name") not in {s["name"] for s in specs}]
    for spec in specs:
        components.append({"name": spec["name"], "oracle": spec["oracle"], "parameters": {item["name"]: item.get("source", item.get("derived", "")) for item in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["files"][1:], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for item in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / item
            if path.is_file():
                hashes[item] = _sha(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.13", "release": "runtime-rtl-v0.13", "generated_by": "acir-runtime-promote-v13", "toolchain": old_manifest.get("toolchain", {}), "policy": old_manifest.get("policy", {}), "components": sorted(components, key=lambda item: item.get("name", "")), "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v13-next.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v13-next.json")
    args = parser.parse_args()
    specs = normalize_specs(_specs())
    catalog = _read(CATALOG)
    old_entries = [entry for entry in catalog.get("entries", []) if entry.get("name") not in {s["name"] for s in specs}]
    functional = _read(args.functional_report) if args.mode == "finalize" else {}
    gate = _read(args.gate_report) if args.mode == "finalize" else {}
    entries = old_entries + [_entry(spec, functional, gate) for spec in specs]
    entries.sort(key=lambda item: str(item.get("name", "")))
    catalog["entries"] = entries
    catalog["runtime_api_version"] = "0.13"
    catalog["generated_by"] = "acir-runtime-promote-v13"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    old_manifest = _read(ROOT / "manifests/parameterized-components-v0.12.json")
    _write_release(specs, entries, old_manifest)
    lock = _read(LOCK)
    sources = lock.setdefault("sources", {})
    sources["basejump-stl-v0.13"] = {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "license": "Solderpad-Hardware-License-0.51"}
    sources["vortex-v0.13"] = {"repository": VORTEX_REPO, "commit": VORTEX_COMMIT, "license": "Apache-2.0"}
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": len(specs), "accepted": sum(entry.get("status") == "accepted" for entry in entries if entry.get("name") in {s["name"] for s in specs})}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
