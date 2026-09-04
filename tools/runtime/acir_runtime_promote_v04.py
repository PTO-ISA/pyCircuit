#!/usr/bin/env python3
"""Promote the reviewed v0.4 candidate batch into the vendored runtime.

The candidate crawler report remains the source of structural evidence; this
small manifest builder adds the human-reviewed semantic contract and stable
wrapper interface, then records every vendored file digest in a release
manifest.  It is intentionally deterministic and idempotent so a completed
functional run can be fed back into the catalog without touching unrelated
entries.
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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.4.json"
FUNCTIONAL = REPO / ".pycircuit_out" / "runtime-functional-validation" / "batch-v04-8.json"
VERIFY = REPO / ".pycircuit_out" / "runtime-catalog-validation" / "catalog-v0.4-final.json"


def _specs() -> list[dict[str, Any]]:
    v = "verilog/vendor-v0.4"
    return [
        {
            "name": "vortex-multiplier", "module": "pyc_runtime_vortex_multiplier",
            "implementation": "VX_multiplier", "source": "vortex-v0.4", "family": "arithmetic",
            "wrapper": "verilog/pyc_runtime_vortex_multiplier.sv",
            "files": ["verilog/pyc_runtime_vortex_multiplier.sv", f"{v}/vortex/hw/rtl/libs/VX_multiplier.sv", f"{v}/vortex/hw/rtl/libs/VX_pipe_register.sv", f"{v}/vortex/hw/rtl/VX_platform.vh", f"{v}/vortex/hw/rtl/VX_scope.vh"],
            "license_file": "licenses/vortex-v0.4/LICENSE",
            "provenance": {"repository": "https://github.com/vortexgpgpu/vortex.git", "commit": "d76b7f24e658867ab57e3942d7c648c3e6af072d", "source_file": "hw/rtl/libs/VX_multiplier.sv", "license": "Apache-2.0"},
            "parameters": [{"name": "A_WIDTH", "source": "A_WIDTH", "default": 8}, {"name": "B_WIDTH", "source": "B_WIDTH", "default": 8}, {"name": "R_WIDTH", "derived": "A_WIDTH + B_WIDTH"}, {"name": "SIGNED", "source": "SIGNED", "default": 0}, {"name": "LATENCY", "source": "LATENCY", "default": 0}],
            "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "enable", "direction": "input", "width": "1"}, {"name": "dataa", "direction": "input", "width": "A_WIDTH"}, {"name": "datab", "direction": "input", "width": "B_WIDTH"}, {"name": "result", "direction": "output", "width": "R_WIDTH"}],
            "oracle": {"id": "vortex-multiplier-v1", "kind": "cycle/combinational", "contract": "result is the signed or unsigned product, optionally delayed by LATENCY enabled clock cycles"},
            "configs": [{"A_WIDTH": 4, "B_WIDTH": 4, "SIGNED": 0, "LATENCY": 0}, {"A_WIDTH": 8, "B_WIDTH": 5, "SIGNED": 0, "LATENCY": 0}, {"A_WIDTH": 5, "B_WIDTH": 6, "SIGNED": 1, "LATENCY": 0}],
        },
        {
            "name": "vortex-lzc", "module": "pyc_runtime_vortex_lzc", "implementation": "VX_lzc", "source": "vortex-v0.4", "family": "reduction", "wrapper": "verilog/pyc_runtime_vortex_lzc.sv",
            "files": ["verilog/pyc_runtime_vortex_lzc.sv", f"{v}/vortex/hw/rtl/libs/VX_find_first.sv", f"{v}/vortex/hw/rtl/libs/VX_lzc.sv", f"{v}/vortex/hw/rtl/VX_platform.vh", f"{v}/vortex/hw/rtl/VX_scope.vh"], "license_file": "licenses/vortex-v0.4/LICENSE",
            "provenance": {"repository": "https://github.com/vortexgpgpu/vortex.git", "commit": "d76b7f24e658867ab57e3942d7c648c3e6af072d", "source_file": "hw/rtl/libs/VX_lzc.sv", "license": "Apache-2.0"},
            "parameters": [{"name": "N", "source": "N", "default": 8}, {"name": "REVERSE", "source": "REVERSE", "default": 0}, {"name": "LOGN", "derived": "(N <= 1) ? 1 : $clog2(N)"}], "ports": [{"name": "data_in", "direction": "input", "width": "N"}, {"name": "data_out", "direction": "output", "width": "LOGN"}, {"name": "valid_out", "direction": "output", "width": "1"}], "oracle": {"id": "vortex-lzc-v1", "kind": "combinational", "contract": "data_out is the leading (REVERSE=0) or trailing (REVERSE=1) zero count and valid_out is false for an all-zero input"}, "configs": [{"N": 1, "REVERSE": 0}, {"N": 8, "REVERSE": 0}, {"N": 8, "REVERSE": 1}, {"N": 13, "REVERSE": 0}],
        },
        {
            "name": "basejump-crossbar", "module": "pyc_runtime_basejump_crossbar", "implementation": "bsg_crossbar_o_by_i", "source": "basejump-stl", "family": "arbitration-interconnect", "wrapper": "verilog/pyc_runtime_basejump_crossbar.sv",
            "files": ["verilog/pyc_runtime_basejump_crossbar.sv", f"{v}/basejump/bsg_misc/bsg_crossbar_o_by_i.sv", f"{v}/basejump/bsg_misc/bsg_mux_one_hot.sv", f"{v}/basejump/bsg_misc/bsg_defines.sv"], "license_file": "licenses/basejump-stl-v0.4/LICENSE",
            "provenance": {"repository": "https://github.com/bespoke-silicon-group/basejump_stl.git", "commit": "b48037e28544425839dbd617d45b1a82631bc1a9", "source_file": "bsg_misc/bsg_crossbar_o_by_i.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "INPUTS", "source": "i_els_p", "default": 2}, {"name": "OUTPUTS", "source": "o_els_p", "default": 2}, {"name": "WIDTH", "source": "width_p", "default": 8}], "ports": [{"name": "inputs", "direction": "input", "width": "[INPUTS-1:0][WIDTH-1:0]"}, {"name": "select_onehot", "direction": "input", "width": "[OUTPUTS-1:0][INPUTS-1:0]"}, {"name": "outputs", "direction": "output", "width": "[OUTPUTS-1:0][WIDTH-1:0]"}], "oracle": {"id": "crossbar-v1", "kind": "combinational", "contract": "each output selects the corresponding one-hot input; an all-zero select produces zero"}, "configs": [{"INPUTS": 2, "OUTPUTS": 2, "WIDTH": 4}, {"INPUTS": 3, "OUTPUTS": 2, "WIDTH": 8}],
        },
        {
            "name": "pulp-stream-register", "module": "pyc_runtime_pulp_stream_register", "implementation": "cc_stream_register", "source": "pulp-common-cells-v0.4", "family": "storage-dataflow", "wrapper": "verilog/pyc_runtime_pulp_stream_register.sv",
            "files": ["verilog/pyc_runtime_pulp_stream_register.sv", f"{v}/pulp/src/cc_stream_register.sv", f"{v}/pulp/include/common_cells/registers.svh", f"{v}/pulp/include/common_cells/deprecated/registers.svh"], "license_file": "licenses/pulp-common-cells-v0.4/LICENSE",
            "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_stream_register.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "DATA_WIDTH", "source": "data_t", "default": 8}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "rst_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"}, {"name": "valid_out", "direction": "output", "width": "1"}, {"name": "ready_out", "direction": "input", "width": "1"}, {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"}], "oracle": {"id": "stream-register-v1", "kind": "cycle", "contract": "one-entry ready/valid register captures on valid && ready, holds under backpressure, and clears synchronously"}, "configs": [{"DATA_WIDTH": 1}, {"DATA_WIDTH": 8}, {"DATA_WIDTH": 16}],
        },
        {
            "name": "pulp-stream-demux", "module": "pyc_runtime_pulp_stream_demux", "implementation": "cc_stream_demux", "source": "pulp-common-cells-v0.4", "family": "dataflow", "wrapper": "verilog/pyc_runtime_pulp_stream_demux.sv", "files": ["verilog/pyc_runtime_pulp_stream_demux.sv", f"{v}/pulp/src/cc_stream_demux.sv"], "license_file": "licenses/pulp-common-cells-v0.4/LICENSE", "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_stream_demux.sv", "license": "Solderpad-Hardware-License-0.51"}, "parameters": [{"name": "OUTPUTS", "source": "NumOup", "default": 2}, {"name": "SELECT_WIDTH", "derived": "(OUTPUTS <= 1) ? 1 : $clog2(OUTPUTS)"}], "ports": [{"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "select_out", "direction": "input", "width": "SELECT_WIDTH"}, {"name": "valid_out", "direction": "output", "width": "OUTPUTS"}, {"name": "ready_out", "direction": "input", "width": "OUTPUTS"}], "oracle": {"id": "stream-demux-v1", "kind": "combinational", "contract": "valid is routed only to select_out and ready propagates back from that output"}, "configs": [{"OUTPUTS": 1}, {"OUTPUTS": 2}, {"OUTPUTS": 3}, {"OUTPUTS": 5}],
        },
        {
            "name": "pulp-stream-mux", "module": "pyc_runtime_pulp_stream_mux", "implementation": "cc_stream_mux", "source": "pulp-common-cells-v0.4", "family": "dataflow", "wrapper": "verilog/pyc_runtime_pulp_stream_mux.sv", "files": ["verilog/pyc_runtime_pulp_stream_mux.sv", f"{v}/pulp/src/cc_pkg.sv", f"{v}/pulp/src/cc_stream_mux.sv", f"{v}/pulp/include/common_cells/assertions.svh"], "license_file": "licenses/pulp-common-cells-v0.4/LICENSE", "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_stream_mux.sv", "license": "Solderpad-Hardware-License-0.51"}, "parameters": [{"name": "INPUTS", "source": "NumInp", "default": 2}, {"name": "DATA_WIDTH", "source": "data_t", "default": 8}, {"name": "SELECT_WIDTH", "derived": "(INPUTS <= 1) ? 1 : $clog2(INPUTS)"}], "ports": [{"name": "data_in", "direction": "input", "width": "[INPUTS-1:0][DATA_WIDTH-1:0]"}, {"name": "valid_in", "direction": "input", "width": "INPUTS"}, {"name": "ready_in", "direction": "output", "width": "INPUTS"}, {"name": "select_in", "direction": "input", "width": "SELECT_WIDTH"}, {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"}, {"name": "valid_out", "direction": "output", "width": "1"}, {"name": "ready_out", "direction": "input", "width": "1"}], "oracle": {"id": "stream-mux-v1", "kind": "combinational", "contract": "selected input data and valid are forwarded; ready is asserted only for the selected input"}, "configs": [{"INPUTS": 1, "DATA_WIDTH": 4}, {"INPUTS": 2, "DATA_WIDTH": 8}, {"INPUTS": 3, "DATA_WIDTH": 5}],
        },
        {
            "name": "pulp-stream-join", "module": "pyc_runtime_pulp_stream_join", "implementation": "cc_stream_join", "source": "pulp-common-cells-v0.4", "family": "dataflow", "wrapper": "verilog/pyc_runtime_pulp_stream_join.sv", "files": ["verilog/pyc_runtime_pulp_stream_join.sv", f"{v}/pulp/src/cc_stream_join.sv", f"{v}/pulp/src/cc_stream_join_dynamic.sv", f"{v}/pulp/include/common_cells/assertions.svh"], "license_file": "licenses/pulp-common-cells-v0.4/LICENSE", "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_stream_join.sv", "license": "Solderpad-Hardware-License-0.51"}, "parameters": [{"name": "INPUTS", "source": "NumInp", "default": 2}], "ports": [{"name": "valid_in", "direction": "input", "width": "INPUTS"}, {"name": "ready_in", "direction": "output", "width": "INPUTS"}, {"name": "valid_out", "direction": "output", "width": "1"}, {"name": "ready_out", "direction": "input", "width": "1"}], "oracle": {"id": "stream-join-v1", "kind": "combinational", "contract": "output valid is asserted only when all selected inputs are valid and ready propagates on a completed output handshake"}, "configs": [{"INPUTS": 1}, {"INPUTS": 2}, {"INPUTS": 3}, {"INPUTS": 5}],
        },
        {
            "name": "pulp-stream-fork", "module": "pyc_runtime_pulp_stream_fork", "implementation": "cc_stream_fork", "source": "pulp-common-cells-v0.4", "family": "dataflow", "wrapper": "verilog/pyc_runtime_pulp_stream_fork.sv", "files": ["verilog/pyc_runtime_pulp_stream_fork.sv", f"{v}/pulp/src/cc_stream_fork.sv", f"{v}/pulp/include/common_cells/assertions.svh", f"{v}/pulp/include/common_cells/registers.svh", f"{v}/pulp/include/common_cells/deprecated/registers.svh"], "license_file": "licenses/pulp-common-cells-v0.4/LICENSE", "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_stream_fork.sv", "license": "Solderpad-Hardware-License-0.51"}, "parameters": [{"name": "OUTPUTS", "source": "NumOup", "default": 2}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "rst_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "valid_out", "direction": "output", "width": "OUTPUTS"}, {"name": "ready_out", "direction": "input", "width": "OUTPUTS"}], "oracle": {"id": "stream-fork-v1", "kind": "cycle", "contract": "one input transaction is presented to all outputs and input ready waits until every output handshakes"}, "configs": [{"OUTPUTS": 1}, {"OUTPUTS": 2}, {"OUTPUTS": 3}, {"OUTPUTS": 5}],
        },
    ]


def _hash(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--functional-report", type=Path, default=FUNCTIONAL)
    ap.add_argument("--verify-report", type=Path, default=VERIFY)
    args = ap.parse_args()
    specs = normalize_specs(_specs())
    old = json.loads(CATALOG.read_text(encoding="utf-8"))
    entries = [e for e in old.get("entries", []) if e.get("name") not in {s["name"] for s in specs}]
    functional: dict[str, Any] = {}
    if args.functional_report.is_file():
        doc = json.loads(args.functional_report.read_text(encoding="utf-8"))
        functional = {str(r.get("name")): r for r in doc.get("results", []) if isinstance(r, dict)}
    verified: dict[str, Any] = {}
    if args.verify_report.is_file():
        doc = json.loads(args.verify_report.read_text(encoding="utf-8"))
        verified = {str(r.get("name")): r for r in doc.get("results", []) if isinstance(r, dict)}
    for s in specs:
        files = list(s["files"])
        missing = [p for p in files + [s["license_file"]] if not (ROOT / p).is_file()]
        fr = functional.get(s["name"], {})
        passed = fr.get("status") == "passed"
        qos = [
            {"parameters": c.get("parameters", {}), "cells": c.get("qor", {}).get("cells"),
             "wires": c.get("qor", {}).get("wires"), "wire_bits": c.get("qor", {}).get("wire_bits")}
            for c in fr.get("cases", []) if isinstance(c, dict)
        ]
        vr = verified.get(s["name"], {})
        entries.append({
            "name": s["name"], "module": s["module"], "implementation": s["implementation"], "source": s["source"], "provider": "github", "status": "accepted", "family": s["family"], "wrapper": s["wrapper"], "files": files,
            "provenance": s["provenance"], "interface": {"wrapper_module": s["module"], "parameters": s["parameters"], "ports": s["ports"]}, "oracle": s["oracle"],
            "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": [p for p in files if p != s["wrapper"]], "license_files": [s["license_file"]], "include_roots": sorted({str(Path(p).parent) for p in files if "include/" in p})},
            "validation": {"status": "passed" if passed else ("pending" if not fr else "failed"), "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.4.json", "configs": s["configs"], "qor": qos, "functional_report": str(args.functional_report.relative_to(ROOT.parent.parent).as_posix()) if args.functional_report.is_absolute() and str(args.functional_report).startswith(str(ROOT.parent.parent)) else str(args.functional_report), "runtime_gate_report": str(args.verify_report.relative_to(ROOT.parent.parent).as_posix()) if args.verify_report.is_absolute() and str(args.verify_report).startswith(str(ROOT.parent.parent)) else str(args.verify_report), "runtime_gate": vr.get("status", "pending")},
            "verification": vr,
        })
    entries.sort(key=lambda e: e["name"])
    old["entries"] = entries
    old["runtime_api_version"] = "0.4"
    old["generated_by"] = "acir-runtime-promote-v04"
    CATALOG.write_text(json.dumps(old, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    sha = {}
    for e in entries:
        if e.get("status") != "accepted":
            continue
        for p in list(e.get("files", [])) + list(e.get("dependency_closure", {}).get("license_files", [])):
            path = ROOT / p
            if path.is_file(): sha[p] = _hash(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.4", "release": "runtime-rtl-v0.4", "generated_by": "acir-runtime-promote-v04", "toolchain": {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}, "policy": {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed"}, "components": [{"name": s["name"], "oracle": s["oracle"], "configs": s["configs"]} for s in specs], "sha256": dict(sorted(sha.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog": str(CATALOG), "manifest": str(MANIFEST), "entries": len(entries), "new_components": len(specs), "functional_pass": sum(functional.get(s["name"], {}).get("status") == "passed" for s in specs), "functional_report": str(args.functional_report)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
