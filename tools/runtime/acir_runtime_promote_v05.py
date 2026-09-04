#!/usr/bin/env python3
"""Promote the v0.5 low-risk primitive batch into the vendored runtime.

The selected candidates are fixed-width or small arithmetic primitives from
the frozen pyCircuit inventory.  This builder is deterministic and records
the reviewed wrapper contract, semantic oracle, complete source closure and
the latest functional/runtime-gate evidence in the catalog and release
manifest.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

from _paths import normalize_specs


REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / "library" / "verilog"
CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.5.json"
FUNCTIONAL = REPO / ".pycircuit_out" / "runtime-functional-validation" / "batch-v05-6.json"
VERIFY = REPO / ".pycircuit_out" / "runtime-catalog-validation" / "batch-v05-6.json"


def _specs() -> list[dict[str, Any]]:
    v = "verilog/vendor-v0.5"
    return [
        {
            "name": "basejump-abs",
            "module": "pyc_runtime_basejump_abs",
            "implementation": "bsg_abs",
            "source": "basejump-stl-v0.5",
            "family": "arithmetic",
            "wrapper": "verilog/pyc_runtime_basejump_abs.v",
            "files": ["verilog/pyc_runtime_basejump_abs.v", f"{v}/basejump/bsg_misc/bsg_abs.sv", f"{v}/basejump/bsg_misc/bsg_defines.sv"],
            "license_file": "licenses/basejump-stl-v0.5/LICENSE",
            "provenance": {"repository": "https://github.com/bespoke-silicon-group/basejump_stl.git", "commit": "b48037e28544425839dbd617d45b1a82631bc1a9", "source_file": "bsg_misc/bsg_abs.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "WIDTH", "source": "width_p", "default": 8}],
            "ports": [{"name": "a", "direction": "input", "width": "WIDTH"}, {"name": "out", "direction": "output", "width": "WIDTH"}],
            "oracle": {"id": "abs-v1", "kind": "combinational", "contract": "out is the WIDTH-bit two's-complement absolute value of a"},
            "configs": [{"WIDTH": 1}, {"WIDTH": 4}, {"WIDTH": 8}, {"WIDTH": 13}],
        },
        {
            "name": "basejump-adder-cin",
            "module": "pyc_runtime_basejump_adder_cin",
            "implementation": "bsg_adder_cin",
            "source": "basejump-stl-v0.5",
            "family": "arithmetic",
            "wrapper": "verilog/pyc_runtime_basejump_adder_cin.v",
            "files": ["verilog/pyc_runtime_basejump_adder_cin.v", f"{v}/basejump/bsg_misc/bsg_adder_cin.sv", f"{v}/basejump/bsg_misc/bsg_defines.sv"],
            "license_file": "licenses/basejump-stl-v0.5/LICENSE",
            "provenance": {"repository": "https://github.com/bespoke-silicon-group/basejump_stl.git", "commit": "b48037e28544425839dbd617d45b1a82631bc1a9", "source_file": "bsg_misc/bsg_adder_cin.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "WIDTH", "source": "width_p", "default": 8}, {"name": "HARDEN", "source": "harden_p", "default": 1}],
            "ports": [{"name": "a", "direction": "input", "width": "WIDTH"}, {"name": "b", "direction": "input", "width": "WIDTH"}, {"name": "cin", "direction": "input", "width": "1"}, {"name": "out", "direction": "output", "width": "WIDTH"}],
            "oracle": {"id": "adder-cin-v1", "kind": "combinational", "contract": "out is the WIDTH-bit unsigned sum of a, b and cin"},
            "configs": [{"WIDTH": 1, "HARDEN": 1}, {"WIDTH": 4, "HARDEN": 1}, {"WIDTH": 8, "HARDEN": 1}, {"WIDTH": 13, "HARDEN": 1}],
        },
        {
            "name": "vortex-adder4",
            "module": "pyc_runtime_vortex_adder4",
            "implementation": "VX_adder4",
            "source": "vortex-v0.5",
            "family": "arithmetic",
            "wrapper": "verilog/pyc_runtime_vortex_adder4.v",
            "files": ["verilog/pyc_runtime_vortex_adder4.v", f"{v}/vortex/hw/rtl/afu/firesim/toy/VX_adder4.v"],
            "license_file": "licenses/vortex-v0.5/LICENSE",
            "provenance": {"repository": "https://github.com/vortexgpgpu/vortex.git", "commit": "5d62846c685ae287f9cd3ddd49f4537c40146eae", "source_file": "hw/rtl/afu/firesim/toy/VX_adder4.v", "license": "Apache-2.0"},
            "parameters": [],
            "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset", "direction": "input", "width": "1"}, {"name": "a", "direction": "input", "width": "4"}, {"name": "b", "direction": "input", "width": "4"}, {"name": "sum", "direction": "output", "width": "5"}],
            "oracle": {"id": "vortex-adder4-v1", "kind": "cycle", "contract": "sum is the registered 5-bit sum of two 4-bit inputs and reset clears it"},
            "configs": [{"WIDTH": 4}],
        },
        {
            "name": "vortex-full-adder",
            "module": "pyc_runtime_vortex_full_adder",
            "implementation": "FullAdder",
            "source": "vortex-v0.5",
            "family": "arithmetic",
            "wrapper": "verilog/pyc_runtime_vortex_full_adder.v",
            "files": ["verilog/pyc_runtime_vortex_full_adder.v", f"{v}/vortex/hw/rtl/libs/VX_csa_32.sv", f"{v}/vortex/hw/VX_config.vh", f"{v}/vortex/hw/VX_types.vh", f"{v}/vortex/hw/rtl/VX_define.vh", f"{v}/vortex/hw/rtl/VX_platform.vh", f"{v}/vortex/hw/rtl/VX_scope.vh"],
            "license_file": "licenses/vortex-v0.5/LICENSE",
            "provenance": {"repository": "https://github.com/vortexgpgpu/vortex.git", "commit": "5d62846c685ae287f9cd3ddd49f4537c40146eae", "source_file": "hw/rtl/libs/VX_csa_32.sv", "license": "Apache-2.0"},
            "parameters": [],
            "ports": [{"name": "a", "direction": "input", "width": "1"}, {"name": "b", "direction": "input", "width": "1"}, {"name": "cin", "direction": "input", "width": "1"}, {"name": "sum", "direction": "output", "width": "1"}, {"name": "cout", "direction": "output", "width": "1"}],
            "oracle": {"id": "full-adder-v1", "kind": "combinational", "contract": "sum and cout implement one-bit full-adder truth table"},
            "configs": [{}],
        },
        {
            "name": "opentitan-secded-22-16-enc",
            "module": "pyc_runtime_opentitan_secded_22_16_enc",
            "implementation": "prim_secded_22_16_enc",
            "source": "opentitan-v0.5",
            "family": "ecc",
            "wrapper": "verilog/pyc_runtime_opentitan_secded_22_16_enc.sv",
            "files": ["verilog/pyc_runtime_opentitan_secded_22_16_enc.sv", f"{v}/opentitan/hw/ip/prim/rtl/prim_secded_22_16_enc.sv"],
            "license_file": "licenses/opentitan-v0.5/LICENSE",
            "provenance": {"repository": "https://github.com/lowRISC/opentitan.git", "commit": "b16f2be75d2f38c62d861208453ed5b81ccf41b0", "source_file": "hw/ip/prim/rtl/prim_secded_22_16_enc.sv", "license": "Apache-2.0"},
            "parameters": [],
            "ports": [{"name": "data_in", "direction": "input", "width": "16"}, {"name": "data_out", "direction": "output", "width": "22"}],
            "oracle": {"id": "secded-22-16-enc-v1", "kind": "combinational", "contract": "data_out is the OpenTitan SECDED(22,16) encoding of data_in"},
            "configs": [{"DATA_WIDTH": 16, "CODE_WIDTH": 22}],
        },
        {
            "name": "opentitan-secded-22-16-dec",
            "module": "pyc_runtime_opentitan_secded_22_16_dec",
            "implementation": "prim_secded_22_16_dec",
            "source": "opentitan-v0.5",
            "family": "ecc",
            "wrapper": "verilog/pyc_runtime_opentitan_secded_22_16_dec.sv",
            "files": ["verilog/pyc_runtime_opentitan_secded_22_16_dec.sv", f"{v}/opentitan/hw/ip/prim/rtl/prim_secded_22_16_dec.sv"],
            "license_file": "licenses/opentitan-v0.5/LICENSE",
            "provenance": {"repository": "https://github.com/lowRISC/opentitan.git", "commit": "b16f2be75d2f38c62d861208453ed5b81ccf41b0", "source_file": "hw/ip/prim/rtl/prim_secded_22_16_dec.sv", "license": "Apache-2.0"},
            "parameters": [],
            "ports": [{"name": "data_in", "direction": "input", "width": "22"}, {"name": "data_out", "direction": "output", "width": "16"}, {"name": "syndrome", "direction": "output", "width": "6"}, {"name": "error", "direction": "output", "width": "2"}],
            "oracle": {"id": "secded-22-16-dec-v1", "kind": "combinational", "contract": "clean codewords decode unchanged, single-bit errors are corrected and reported"},
            "configs": [{"DATA_WIDTH": 16, "CODE_WIDTH": 22}],
        },
    ]


def _hash(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--functional-report", type=Path, default=FUNCTIONAL)
    parser.add_argument("--verify-report", type=Path, default=VERIFY)
    args = parser.parse_args()
    specs = normalize_specs(_specs())
    old = json.loads(CATALOG.read_text(encoding="utf-8"))
    names = {s["name"] for s in specs}
    entries = [entry for entry in old.get("entries", []) if entry.get("name") not in names]
    functional_doc = json.loads(args.functional_report.read_text(encoding="utf-8")) if args.functional_report.is_file() else {}
    verify_doc = json.loads(args.verify_report.read_text(encoding="utf-8")) if args.verify_report.is_file() else {}
    functional = {str(row.get("name")): row for row in functional_doc.get("results", []) if isinstance(row, dict)}
    verified = {str(row.get("name")): row for row in verify_doc.get("results", []) if isinstance(row, dict)}
    for spec in specs:
        all_files = list(spec["files"]) + [spec["license_file"]]
        missing = [item for item in all_files if not (ROOT / item).is_file()]
        fr = functional.get(spec["name"], {})
        vr = verified.get(spec["name"], {})
        qos = [{"parameters": case.get("parameters", {}), "cells": case.get("qor", {}).get("cells"), "wires": case.get("qor", {}).get("wires"), "wire_bits": case.get("qor", {}).get("wire_bits")} for case in fr.get("cases", []) if isinstance(case, dict)]
        entries.append({
            "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"], "source": spec["source"], "provider": "github", "status": "accepted", "family": spec["family"], "wrapper": spec["wrapper"], "files": list(spec["files"]), "provenance": spec["provenance"],
            "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]}, "oracle": spec["oracle"],
            "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": [item for item in spec["files"] if item != spec["wrapper"]], "license_files": [spec["license_file"]], "include_roots": sorted({str(Path(item).parent) for item in spec["files"] if "/include/" in item})},
            "validation": {"status": "passed" if fr.get("status") == "passed" else ("pending" if not fr else "failed"), "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.5.json", "configs": spec["configs"], "qor": qos, "functional_report": str(args.functional_report.relative_to(REPO).as_posix()) if args.functional_report.is_absolute() else str(args.functional_report), "runtime_gate_report": str(args.verify_report.relative_to(REPO).as_posix()) if args.verify_report.is_absolute() else str(args.verify_report), "runtime_gate": vr.get("status", "pending")},
            "verification": vr,
        })
    entries.sort(key=lambda entry: entry["name"])
    old["entries"] = entries
    old["runtime_api_version"] = "0.5"
    old["generated_by"] = "acir-runtime-promote-v05"
    CATALOG.write_text(json.dumps(old, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    hashes: dict[str, str] = {}
    for entry in entries:
        if entry.get("status") != "accepted":
            continue
        for item in list(entry.get("files", [])) + list(entry.get("dependency_closure", {}).get("license_files", [])):
            path = ROOT / item
            if path.is_file():
                hashes[item] = _hash(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.5", "release": "runtime-rtl-v0.5", "generated_by": "acir-runtime-promote-v05", "toolchain": {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}, "policy": {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed"}, "components": [{"name": spec["name"], "oracle": spec["oracle"], "configs": spec["configs"]} for spec in specs], "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog": str(CATALOG), "manifest": str(MANIFEST), "entries": len(entries), "new_components": len(specs), "functional_pass": sum(functional.get(spec["name"], {}).get("status") == "passed" for spec in specs)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
