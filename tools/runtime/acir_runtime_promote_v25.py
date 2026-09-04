#!/usr/bin/env python3
"""Promote the final independent candidates as runtime RTL v0.25.

The six remaining fixed-width/helper candidates are covered by existing
parameterized entries (recorded by ``acir_runtime_candidate_disposition``).
This release contains the two candidates that have a stable public contract:
BaseJump banked-crossbar arbitration control and Vortex BF16 widening.
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
MANIFEST = ROOT / "manifests/parameterized-components-v0.25.json"
LOCK = ROOT / "catalog.lock.json"


def _specs() -> list[dict[str, Any]]:
    base = "verilog/vendor-v0.25/basejump"
    crossbar_files = [
        f"{base}/bsg_mem/bsg_mem_banked_crossbar.sv",
        f"{base}/bsg_misc/bsg_defines.sv",
        f"{base}/bsg_misc/bsg_transpose.sv",
        f"{base}/bsg_misc/bsg_arb_fixed.sv",
        f"{base}/bsg_misc/bsg_priority_encode_one_hot_out.sv",
        f"{base}/bsg_misc/bsg_scan.sv",
        f"{base}/bsg_misc/bsg_round_robin_arb.sv",
    ]
    return [
        {
            "name": "basejump-crossbar-control",
            "module": "pyc_runtime_basejump_crossbar_control",
            "implementation": "bsg_mem_banked_crossbar_control_o_by_i",
            "source": "basejump-stl-v0.25",
            "provider": "github",
            "family": "arbitration-interconnect",
            "wrapper": "verilog/pyc_runtime_basejump_crossbar_control.sv",
            "repository": "https://github.com/bespoke-silicon-group/basejump_stl.git",
            "commit": "b48037e28544425839dbd617d45b1a82631bc1a9",
            "source_file": "bsg_mem/bsg_mem_banked_crossbar.sv",
            "license": "Solderpad-Hardware-License-0.51",
            "license_file": "licenses/basejump-stl-v0.5/LICENSE",
            "source_files": crossbar_files,
            "include_roots": [
                "verilog/vendor-v0.25/basejump/bsg_misc",
                "verilog/vendor-v0.25/basejump/bsg_mem",
            ],
            "parameters": [
                {"name": "INPUTS", "source": "i_els_p", "default": 2},
                {"name": "OUTPUTS", "source": "o_els_p", "default": 4},
                {"name": "RR_LO_HI", "source": "rr_lo_hi_p", "default": 1},
                {"name": "SELECT_WIDTH", "source": "lg_o_els_lp", "derived": "safe_clog2(OUTPUTS)"},
            ],
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "reset", "direction": "input", "width": "1"},
                {"name": "reverse_priority", "direction": "input", "width": "1"},
                {"name": "valid", "direction": "input", "width": "INPUTS"},
                {"name": "select", "direction": "input", "width": "INPUTS*SELECT_WIDTH"},
                {"name": "yumi", "direction": "output", "width": "INPUTS"},
                {"name": "ready", "direction": "input", "width": "OUTPUTS"},
                {"name": "output_valid", "direction": "output", "width": "OUTPUTS"},
                {"name": "grants_onehot", "direction": "output", "width": "OUTPUTS*INPUTS"},
            ],
            "oracle": {
                "id": "basejump-crossbar-control-v1",
                "kind": "combinational-arbitration",
                "contract": "each ready destination grants at most one valid requester; fixed-low priority resolves contention to the lowest requester and yumi is the transpose of grants",
            },
            "configs": [
                {"INPUTS": 2, "OUTPUTS": 4, "RR_LO_HI": 1},
                {"INPUTS": 4, "OUTPUTS": 4, "RR_LO_HI": 1},
            ],
        },
        {
            "name": "vortex-bf16-to-fp32",
            "module": "pyc_runtime_vortex_bf16_to_fp32",
            "implementation": "bf16_to_fp32",
            "source": "vortex-v0.25",
            "provider": "github",
            "family": "arithmetic-format-conversion",
            "wrapper": "verilog/pyc_runtime_vortex_bf16_to_fp32.sv",
            "repository": "https://github.com/vortexgpgpu/vortex.git",
            "commit": "d76b7f24e658867ab57e3942d7c648c3e6af072d",
            "source_file": "hw/rtl/tcu/dsp/VX_tcu_fedp_dsp.sv (bf16_to_fp32 leaf)",
            "license": "Apache-2.0",
            "license_file": "licenses/vortex-v0.4/LICENSE",
            "source_files": ["verilog/vendor-v0.25/vortex/hw/rtl/tcu/dsp/bf16_to_fp32.sv"],
            "include_roots": ["verilog/vendor-v0.25/vortex/hw/rtl/tcu/dsp"],
            "parameters": [],
            "ports": [
                {"name": "bf16_in", "direction": "input", "width": "16"},
                {"name": "fp32_out", "direction": "output", "width": "32"},
            ],
            "oracle": {
                "id": "vortex-bf16-to-fp32-v1",
                "kind": "combinational",
                "contract": "fp32_out preserves the BF16 sign and exponent and appends sixteen zero fraction bits, including signed zero, infinities and NaN payloads",
            },
            "configs": [{}],
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


def _result(report: Mapping[str, Any], name: str) -> dict[str, Any]:
    return next((dict(item) for item in report.get("results", []) if isinstance(item, Mapping) and str(item.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any], *, staging: bool) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(x) for x in spec["source_files"]]]
    license_file = str(spec["license_file"])
    missing = [x for x in [*files, license_file] if not (ROOT / x).is_file()]
    f = _result(functional, str(spec["name"]))
    g = _result(gate, str(spec["name"]))
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
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": files[1:], "license_files": [license_file], "include_roots": spec["include_roots"]},
        "validation": {
            "status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys",
            "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.25.json",
            "configs": spec["configs"], "functional_report": ".pycircuit_out/runtime-functional-validation/v25-final.json",
            "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v25-final.json",
            "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qor,
        },
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v25-final.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v25-final.json")
    args = parser.parse_args()
    specs = normalize_specs(_specs()); functional = _read(args.functional_report) if args.mode == "finalize" else {}; gate = _read(args.gate_report) if args.mode == "finalize" else {}
    catalog = _read(CATALOG); entries = [e for e in catalog.get("entries", []) if str(e.get("name")) not in {str(s["name"]) for s in specs}]
    for spec in specs:
        entries.append(_entry(spec, functional, gate, staging=args.mode == "prepare"))
    entries.sort(key=lambda e: str(e.get("name", "")))
    catalog.update({"entries": entries, "runtime_api_version": "0.25", "generated_by": "acir-runtime-promote-v25"})
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old = _read(ROOT / "manifests/parameterized-components-v0.24.json")
    components = [c for c in old.get("components", []) if str(c.get("name")) not in {str(s["name"]) for s in specs}]
    for spec in specs:
        components.append({"name": spec["name"], "oracle": spec["oracle"], "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": [spec["wrapper"], *spec["source_files"]], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for path_text in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(path_text)
            if path.is_file(): hashes[str(path_text)] = _sha(path)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.25", "release": "runtime-rtl-v0.25", "generated_by": "acir-runtime-promote-v25", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda c: str(c.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    lock = _read(LOCK); sources = lock.setdefault("sources", {})
    for spec in specs:
        sources[spec["source"]] = {"repository": spec["repository"], "commit": spec["commit"], "license": spec["license"]}
    LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": len(specs), "accepted": sum(e.get("status") == "accepted" for e in entries if e.get("name") in {s["name"] for s in specs})}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
