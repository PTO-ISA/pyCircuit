#!/usr/bin/env python3
"""Promote BaseJump round-robin FIFO-to-FIFO as runtime RTL v0.22."""

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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.22.json"


def _spec() -> dict[str, Any]:
    base = "verilog/vendor-v0.5/basejump"
    return {
        "name": "basejump-rr-fifo-to-fifo",
        "module": "pyc_runtime_basejump_rr_fifo_to_fifo",
        "implementation": "bsg_round_robin_fifo_to_fifo",
        "source": "basejump-stl-v0.5",
        "provider": "github",
        "family": "arbitration-interconnect",
        "wrapper": "verilog/pyc_runtime_basejump_rr_fifo_to_fifo.sv",
        "source_files": [
            f"{base}/bsg_dataflow/bsg_round_robin_fifo_to_fifo.sv",
            f"{base}/bsg_dataflow/bsg_make_2D_array.sv",
            f"{base}/bsg_misc/bsg_circular_ptr.sv",
            f"{base}/bsg_misc/bsg_encode_one_hot.sv",
            f"{base}/bsg_misc/bsg_popcount.sv",
            f"{base}/bsg_misc/bsg_rotate_right.sv",
            f"{base}/bsg_misc/bsg_scan.sv",
            f"{base}/bsg_misc/bsg_thermometer_count.sv",
            f"{base}/bsg_misc/bsg_defines.sv",
        ],
        "repository": "https://github.com/bespoke-silicon-group/basejump_stl.git",
        "commit": "b48037e28544425839dbd617d45b1a82631bc1a9",
        "source_file": "bsg_dataflow/bsg_round_robin_fifo_to_fifo.sv",
        "license": "Solderpad-Hardware-License-0.51",
        "license_file": "licenses/basejump-stl-v0.5/LICENSE",
        "parameters": [
            {"name": "NUM_INPUTS", "source": "num_in_p", "default": 4},
            {"name": "DATA_WIDTH", "source": "width_p", "default": 8},
            {"name": "NUM_OUTPUTS", "source": "num_out_p", "default": 1},
            {"name": "IN_CHANNEL_COUNT_MASK", "source": "in_channel_count_mask_p", "default": 15},
            {"name": "OUT_CHANNEL_COUNT_MASK", "source": "out_channel_count_mask_p", "default": 1},
        ],
        "ports": [
            {"name": "clk", "direction": "input", "width": "1"},
            {"name": "reset", "direction": "input", "width": "1"},
            {"name": "input_valid", "direction": "input", "width": "NUM_INPUTS"},
            {"name": "input_data", "direction": "input", "width": "NUM_INPUTS*DATA_WIDTH"},
            {"name": "input_yumi", "direction": "output", "width": "NUM_INPUTS"},
            {"name": "input_top_channel", "direction": "input", "width": "max(1,clog2(NUM_INPUTS))"},
            {"name": "output_top_channel", "direction": "input", "width": "max(1,clog2(NUM_OUTPUTS))"},
            {"name": "output_valid", "direction": "output", "width": "NUM_OUTPUTS"},
            {"name": "output_data", "direction": "output", "width": "NUM_OUTPUTS*DATA_WIDTH"},
            {"name": "output_ready", "direction": "input", "width": "NUM_OUTPUTS"},
        ],
        "oracle": {
            "id": "basejump-rr-fifo-to-fifo-v1",
            "kind": "cycle/ready-valid",
            "contract": "a ready output consumes one valid input, emits the matching data and one-hot yumi, and advances the blocking round-robin input pointer",
        },
        "configs": [
            {"NUM_INPUTS": 2, "DATA_WIDTH": 4, "NUM_OUTPUTS": 1,
             "IN_CHANNEL_COUNT_MASK": 3, "OUT_CHANNEL_COUNT_MASK": 1},
            {"NUM_INPUTS": 4, "DATA_WIDTH": 8, "NUM_OUTPUTS": 1,
             "IN_CHANNEL_COUNT_MASK": 15, "OUT_CHANNEL_COUNT_MASK": 1},
            {"NUM_INPUTS": 5, "DATA_WIDTH": 8, "NUM_OUTPUTS": 1,
             "IN_CHANNEL_COUNT_MASK": 31, "OUT_CHANNEL_COUNT_MASK": 1},
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
            qor.append({"parameters": case.get("parameters", {}),
                        **{key: q.get(key) for key in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"],
        "source": spec["source"], "provider": spec["provider"],
        "status": "accepted" if accepted else "pending", "family": spec["family"],
        "wrapper": spec["wrapper"], "files": files,
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
            "manifest": "manifests/parameterized-components-v0.22.json", "configs": spec["configs"],
            "functional_report": ".pycircuit_out/runtime-functional-validation/v22-basejump-rr-fifo-to-fifo.json",
            "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v22-basejump-rr-fifo-to-fifo.json",
            "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"),
            "qor": qor,
        },
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending",
                         "top": spec["module"], "files": files,
                         "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path,
                        default=REPO / ".pycircuit_out/runtime-functional-validation/v22-basejump-rr-fifo-to-fifo.json")
    parser.add_argument("--gate-report", type=Path,
                        default=REPO / ".pycircuit_out/runtime-catalog-validation/v22-basejump-rr-fifo-to-fifo.json")
    args = parser.parse_args()
    spec = normalize_spec(_spec())
    functional = _read(args.functional_report) if args.mode == "finalize" else {}
    gate = _read(args.gate_report) if args.mode == "finalize" else {}
    catalog = _read(CATALOG)
    entries = [entry for entry in catalog.get("entries", []) if str(entry.get("name")) != spec["name"]]
    entries.append(_entry(spec, functional, gate))
    entries.sort(key=lambda entry: str(entry.get("name", "")))
    catalog["entries"] = entries
    catalog["runtime_api_version"] = "0.22"
    catalog["generated_by"] = "acir-runtime-promote-v22"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old = _read(ROOT / "manifests/parameterized-components-v0.21.json")
    components = [component for component in old.get("components", [])
                  if str(component.get("name")) != spec["name"]]
    components.append({"name": spec["name"], "oracle": spec["oracle"],
                       "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]},
                       "configs": spec["configs"],
                       "source": {"repository": spec["repository"], "commit": spec["commit"],
                                  "files": spec["source_files"], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        paths = [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]
        for path_text in paths:
            path = ROOT / str(path_text)
            if path.is_file():
                hashes[str(path_text)] = _sha(path)
    MANIFEST.write_text(json.dumps({
        "schema": "acir-runtime-parameterized-components-v0.22", "release": "runtime-rtl-v0.22",
        "generated_by": "acir-runtime-promote-v22", "toolchain": old.get("toolchain", {}),
        "policy": old.get("policy", {}),
        "components": sorted(components, key=lambda component: str(component.get("name", ""))),
        "sha256": dict(sorted(hashes.items())),
    }, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    promoted = next(entry for entry in entries if str(entry.get("name")) == str(spec["name"]))
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": 1,
                      "accepted": int(promoted.get("status") == "accepted")}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
