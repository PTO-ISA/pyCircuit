#!/usr/bin/env python3
"""Promote the reviewed Vortex/PULP/OpenTitan v0.6 runtime batch.

Every promoted component has a stable wrapper, a bounded functional oracle,
and a vendored source closure.  The CDC FIFO is promoted only with its
explicit dual-clock oracle; it is not treated as an ordinary single-clock
ready/valid block.
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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.6.json"
FUNCTIONAL = REPO / ".pycircuit_out" / "runtime-functional-validation" / "batch-v06-10.json"
VERIFY = REPO / ".pycircuit_out" / "runtime-catalog-validation" / "batch-v06-10.json"


def _include_roots(files: list[str]) -> list[str]:
    roots: set[str] = set()
    for item in files:
        parts = Path(item).parts
        if "include" in parts:
            roots.add(str(Path(*parts[: parts.index("include") + 1])).replace("\\", "/"))
    return sorted(roots)


def _specs() -> list[dict[str, Any]]:
    v = "verilog/vendor-v0.6"
    vortex_headers = [
        f"{v}/vortex/hw/VX_config.vh", f"{v}/vortex/hw/VX_types.vh",
        f"{v}/vortex/hw/rtl/VX_define.vh", f"{v}/vortex/hw/rtl/VX_platform.vh",
        f"{v}/vortex/hw/rtl/VX_scope.vh",
    ]
    return [
        {
            "name": "vortex-elastic-buffer", "module": "pyc_runtime_vortex_elastic_buffer",
            "implementation": "VX_elastic_buffer", "source": "vortex-v0.6", "family": "dataflow",
            "wrapper": "verilog/pyc_runtime_vortex_elastic_buffer.sv",
            "files": [
                "verilog/pyc_runtime_vortex_elastic_buffer.sv",
                f"{v}/vortex/hw/rtl/libs/VX_elastic_buffer.sv",
                f"{v}/vortex/hw/rtl/libs/VX_stream_buffer.sv",
                f"{v}/vortex/hw/rtl/libs/VX_pipe_buffer.sv",
                f"{v}/vortex/hw/rtl/libs/VX_pipe_register.sv",
                f"{v}/vortex/hw/rtl/libs/VX_fifo_queue.sv",
                f"{v}/vortex/hw/rtl/libs/VX_pending_size.sv",
                f"{v}/vortex/hw/rtl/libs/VX_dp_ram.sv",
                f"{v}/vortex/hw/rtl/libs/VX_async_ram_patch.sv",
                f"{v}/vortex/hw/rtl/libs/VX_placeholder.sv", *vortex_headers,
            ],
            "license_file": "licenses/vortex-v0.6-LICENSE",
            "provenance": {"repository": "https://github.com/vortexgpgpu/vortex.git", "commit": "5d62846c685ae287f9cd3ddd49f4537c40146eae", "source_file": "hw/rtl/libs/VX_elastic_buffer.sv", "license": "Apache-2.0"},
            "parameters": [{"name": "DATA_WIDTH", "source": "DATAW", "default": 8}, {"name": "SIZE", "source": "SIZE", "default": 2}, {"name": "OUT_REG", "source": "OUT_REG", "default": 0}, {"name": "LUTRAM", "source": "LUTRAM", "default": 0}],
            "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset", "direction": "input", "width": "1"}, {"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"}, {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"}, {"name": "ready_out", "direction": "input", "width": "1"}, {"name": "valid_out", "direction": "output", "width": "1"}],
            "oracle": {"id": "elastic-buffer-v1", "kind": "cycle", "contract": "accepted ready/valid words are held in order through backpressure and emitted once ready_out is asserted"},
            "configs": [{"DATA_WIDTH": 4, "SIZE": 2}, {"DATA_WIDTH": 8, "SIZE": 4}],
        },
        {
            "name": "vortex-skid-buffer", "module": "pyc_runtime_vortex_skid_buffer", "implementation": "VX_skid_buffer", "source": "vortex-v0.6", "family": "dataflow",
            "wrapper": "verilog/pyc_runtime_vortex_skid_buffer.sv",
            "files": ["verilog/pyc_runtime_vortex_skid_buffer.sv", f"{v}/vortex/hw/rtl/libs/VX_skid_buffer.sv", f"{v}/vortex/hw/rtl/libs/VX_stream_buffer.sv", f"{v}/vortex/hw/rtl/libs/VX_toggle_buffer.sv", *vortex_headers],
            "license_file": "licenses/vortex-v0.6-LICENSE",
            "provenance": {"repository": "https://github.com/vortexgpgpu/vortex.git", "commit": "5d62846c685ae287f9cd3ddd49f4537c40146eae", "source_file": "hw/rtl/libs/VX_skid_buffer.sv", "license": "Apache-2.0"},
            "parameters": [{"name": "DATA_WIDTH", "source": "DATAW", "default": 8}, {"name": "PASSTHRU", "source": "PASSTHRU", "default": 0}, {"name": "HALF_BW", "source": "HALF_BW", "default": 0}, {"name": "OUT_REG", "source": "OUT_REG", "default": 0}],
            "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset", "direction": "input", "width": "1"}, {"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"}, {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"}, {"name": "ready_out", "direction": "input", "width": "1"}, {"name": "valid_out", "direction": "output", "width": "1"}],
            "oracle": {"id": "skid-buffer-v1", "kind": "cycle", "contract": "one ready/valid word is retained during downstream backpressure and drains without corruption"},
            "configs": [{"DATA_WIDTH": 8, "HALF_BW": 0}, {"DATA_WIDTH": 8, "HALF_BW": 1}],
        },
        {
            "name": "pulp-spill-register", "module": "pyc_runtime_pulp_spill_register", "implementation": "cc_spill_register", "source": "pulp-common-cells-v0.6", "family": "dataflow",
            "wrapper": "verilog/pyc_runtime_pulp_spill_register.sv",
            "files": ["verilog/pyc_runtime_pulp_spill_register.sv", f"{v}/pulp_common_cells/src/cc_spill_register.sv", f"{v}/pulp_common_cells/src/cc_spill_register_flushable.sv", f"{v}/pulp_common_cells/include/common_cells/assertions.svh", f"{v}/pulp_common_cells/include/common_cells/registers.svh", f"{v}/pulp_common_cells/include/common_cells/deprecated/registers.svh"],
            "license_file": "licenses/pulp-common-cells-v0.6-LICENSE",
            "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_spill_register.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "DATA_WIDTH", "source": "data_t", "default": 8}, {"name": "BYPASS", "source": "Bypass", "default": 0}],
            "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "rst_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"}, {"name": "valid_out", "direction": "output", "width": "1"}, {"name": "ready_out", "direction": "input", "width": "1"}, {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"}],
            "oracle": {"id": "spill-register-v1", "kind": "cycle", "contract": "a two-entry spill register accepts and preserves a word under ready/valid backpressure"},
            "configs": [{"DATA_WIDTH": 4}, {"DATA_WIDTH": 8}],
        },
        {
            "name": "pulp-spill-register-flushable", "module": "pyc_runtime_pulp_spill_register_flushable", "implementation": "cc_spill_register_flushable", "source": "pulp-common-cells-v0.6", "family": "dataflow",
            "wrapper": "verilog/pyc_runtime_pulp_spill_register_flushable.sv",
            "files": ["verilog/pyc_runtime_pulp_spill_register_flushable.sv", f"{v}/pulp_common_cells/src/cc_spill_register_flushable.sv", f"{v}/pulp_common_cells/include/common_cells/assertions.svh", f"{v}/pulp_common_cells/include/common_cells/registers.svh", f"{v}/pulp_common_cells/include/common_cells/deprecated/registers.svh"],
            "license_file": "licenses/pulp-common-cells-v0.6-LICENSE",
            "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_spill_register_flushable.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "DATA_WIDTH", "source": "data_t", "default": 8}, {"name": "BYPASS", "source": "Bypass", "default": 0}],
            "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "rst_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "flush", "direction": "input", "width": "1"}, {"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"}, {"name": "valid_out", "direction": "output", "width": "1"}, {"name": "ready_out", "direction": "input", "width": "1"}, {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"}],
            "oracle": {"id": "spill-register-flushable-v1", "kind": "cycle", "contract": "spill register preserves a word under backpressure and flush drains stored state"},
            "configs": [{"DATA_WIDTH": 4}, {"DATA_WIDTH": 8}],
        },
        {
            "name": "pulp-stream-fork-dynamic", "module": "pyc_runtime_pulp_stream_fork_dynamic", "implementation": "cc_stream_fork_dynamic", "source": "pulp-common-cells-v0.6", "family": "dataflow", "wrapper": "verilog/pyc_runtime_pulp_stream_fork_dynamic.sv",
            "files": ["verilog/pyc_runtime_pulp_stream_fork_dynamic.sv", f"{v}/pulp_common_cells/src/cc_stream_fork_dynamic.sv", f"{v}/pulp_common_cells/src/cc_stream_fork.sv", f"{v}/pulp_common_cells/include/common_cells/assertions.svh", f"{v}/pulp_common_cells/include/common_cells/registers.svh", f"{v}/pulp_common_cells/include/common_cells/deprecated/registers.svh"], "license_file": "licenses/pulp-common-cells-v0.6-LICENSE",
            "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_stream_fork_dynamic.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "OUTPUTS", "source": "NumOup", "default": 2}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "rst_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "valid_in", "direction": "input", "width": "1"}, {"name": "ready_in", "direction": "output", "width": "1"}, {"name": "select_mask", "direction": "input", "width": "OUTPUTS"}, {"name": "select_valid", "direction": "input", "width": "1"}, {"name": "select_ready", "direction": "output", "width": "1"}, {"name": "valid_out", "direction": "output", "width": "OUTPUTS"}, {"name": "ready_out", "direction": "input", "width": "OUTPUTS"}],
            "oracle": {"id": "stream-fork-dynamic-v1", "kind": "cycle", "contract": "selected outputs handshake exactly once and input ready asserts only after all selected outputs complete"}, "configs": [{"OUTPUTS": 2}, {"OUTPUTS": 3}, {"OUTPUTS": 5}],
        },
        {
            "name": "pulp-stream-join-dynamic", "module": "pyc_runtime_pulp_stream_join_dynamic", "implementation": "cc_stream_join_dynamic", "source": "pulp-common-cells-v0.6", "family": "dataflow", "wrapper": "verilog/pyc_runtime_pulp_stream_join_dynamic.sv",
            "files": ["verilog/pyc_runtime_pulp_stream_join_dynamic.sv", f"{v}/pulp_common_cells/src/cc_stream_join_dynamic.sv", f"{v}/pulp_common_cells/include/common_cells/assertions.svh"], "license_file": "licenses/pulp-common-cells-v0.6-LICENSE",
            "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_stream_join_dynamic.sv", "license": "Solderpad-Hardware-License-0.51"},
            "parameters": [{"name": "INPUTS", "source": "NumInp", "default": 2}], "ports": [{"name": "valid_in", "direction": "input", "width": "INPUTS"}, {"name": "ready_in", "direction": "output", "width": "INPUTS"}, {"name": "select_mask", "direction": "input", "width": "INPUTS"}, {"name": "valid_out", "direction": "output", "width": "1"}, {"name": "ready_out", "direction": "input", "width": "1"}],
            "oracle": {"id": "stream-join-dynamic-v1", "kind": "combinational", "contract": "output valid requires every selected input valid and ready returns only on a completed output handshake"}, "configs": [{"INPUTS": 2}, {"INPUTS": 3}, {"INPUTS": 5}],
        },
        {
            "name": "pulp-cdc-fifo-gray", "module": "pyc_runtime_pulp_cdc_fifo_gray",
            "implementation": "cc_cdc_fifo_gray", "source": "pulp-common-cells-v0.6",
            "family": "cdc", "wrapper": "verilog/pyc_runtime_pulp_cdc_fifo_gray.sv",
            "files": [
                "verilog/pyc_runtime_pulp_cdc_fifo_gray.sv",
                f"{v}/pulp_common_cells/src/cc_cdc_fifo_gray.sv",
                f"{v}/pulp_common_cells/src/cc_binary_to_gray.sv",
                f"{v}/pulp_common_cells/src/cc_gray_to_binary.sv",
                f"{v}/pulp_common_cells/src/cc_spill_register.sv",
                f"{v}/pulp_common_cells/src/cc_spill_register_flushable.sv",
                f"{v}/pulp_common_cells/include/common_cells/assertions.svh",
                f"{v}/pulp_common_cells/include/common_cells/registers.svh",
                f"{v}/pulp_common_cells/include/common_cells/deprecated/registers.svh",
                f"{v}/tech_cells_generic/src/rtl/tc_sync.sv",
            ],
            "license_file": "licenses/pulp-common-cells-v0.6-LICENSE",
            "license_files": ["licenses/pulp-common-cells-v0.6-LICENSE", "licenses/tech-cells-generic-v0.6-LICENSE"],
            "provenance": {"repository": "https://github.com/pulp-platform/common_cells.git", "commit": "db42769334b4589b4b3fc671b34513bdb98be565", "source_file": "src/cc_cdc_fifo_gray.sv", "license": "Solderpad-Hardware-License-0.51", "dependencies": [{"repository": "https://github.com/pulp-platform/tech_cells_generic.git", "commit": "55cb54513e2d426be5992d311cb9d5dbcad10c78", "source_file": "src/rtl/tc_sync.sv", "license": "Solderpad-Hardware-License-0.51"}]},
            "parameters": [{"name": "DATA_WIDTH", "source": "Width/data_t", "default": 8}, {"name": "LOG_DEPTH", "source": "LogDepth", "default": 2}, {"name": "SYNC_STAGES", "source": "SyncStages", "default": 2}],
            "ports": [
                {"name": "src_rst_n", "direction": "input", "width": "1"}, {"name": "src_clk", "direction": "input", "width": "1"}, {"name": "src_data", "direction": "input", "width": "DATA_WIDTH"}, {"name": "src_valid", "direction": "input", "width": "1"}, {"name": "src_ready", "direction": "output", "width": "1"},
                {"name": "dst_rst_n", "direction": "input", "width": "1"}, {"name": "dst_clk", "direction": "input", "width": "1"}, {"name": "dst_data", "direction": "output", "width": "DATA_WIDTH"}, {"name": "dst_valid", "direction": "output", "width": "1"}, {"name": "dst_ready", "direction": "input", "width": "1"},
            ],
            "oracle": {"id": "cdc-fifo-gray-v1", "kind": "dual-clock-cycle", "contract": "a source-domain ready/valid word crosses the gray-pointer FIFO and is observed unchanged in the destination domain after synchronized reset; source and destination resets are asserted together"},
            "configs": [{"DATA_WIDTH": 2, "LOG_DEPTH": 2, "SYNC_STAGES": 2}, {"DATA_WIDTH": 3, "LOG_DEPTH": 2, "SYNC_STAGES": 2}],
        },
    ]


def _ecc_specs() -> list[dict[str, Any]]:
    v = "verilog/vendor-v0.6/opentitan/hw/ip/prim/rtl"
    out: list[dict[str, Any]] = []
    for data, code in ((22, 28), (32, 39)):
        for direction in ("enc", "dec"):
            name = f"opentitan-secded-{code}-{data}-{direction}"
            module = f"pyc_runtime_opentitan_secded_{code}_{data}_{direction}"
            ports = ([{"name": "data_in", "direction": "input", "width": str(data)}, {"name": "data_out", "direction": "output", "width": str(code)}] if direction == "enc" else [{"name": "data_in", "direction": "input", "width": str(code)}, {"name": "data_out", "direction": "output", "width": str(data)}, {"name": "syndrome", "direction": "output", "width": str(code - data)}, {"name": "error", "direction": "output", "width": "2"}])
            out.append({"name": name, "module": module, "implementation": f"prim_secded_{code}_{data}_{direction}", "source": "opentitan-v0.6", "family": "ecc", "wrapper": f"verilog/{module}.sv", "files": [f"verilog/{module}.sv", f"{v}/prim_secded_{code}_{data}_{direction}.sv"], "license_file": "licenses/opentitan-v0.6-LICENSE", "provenance": {"repository": "https://github.com/lowRISC/opentitan.git", "commit": "b16f2be75d2f38c62d861208453ed5b81ccf41b0", "source_file": f"hw/ip/prim/rtl/prim_secded_{code}_{data}_{direction}.sv", "license": "Apache-2.0"}, "parameters": [], "ports": ports, "oracle": {"id": f"secded-{code}-{data}-{direction}-v1", "kind": "combinational", "contract": f"OpenTitan SECDED({code},{data}) {'encoding' if direction == 'enc' else 'decoding with single-bit correction and error reporting'}"}, "configs": [{"DATA_WIDTH": data, "CODE_WIDTH": code}]})
    return out


def _hash(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--functional-report", type=Path, default=FUNCTIONAL)
    ap.add_argument("--verify-report", type=Path, default=VERIFY)
    args = ap.parse_args()
    specs = normalize_specs(_specs() + _ecc_specs())
    old = json.loads(CATALOG.read_text(encoding="utf-8"))
    names = {s["name"] for s in specs}
    entries = [e for e in old.get("entries", []) if e.get("name") not in names]
    fdoc = json.loads(args.functional_report.read_text(encoding="utf-8")) if args.functional_report.is_file() else {}
    vdoc = json.loads(args.verify_report.read_text(encoding="utf-8")) if args.verify_report.is_file() else {}
    functional = {str(r.get("name")): r for r in fdoc.get("results", []) if isinstance(r, dict)}
    verified = {str(r.get("name")): r for r in vdoc.get("results", []) if isinstance(r, dict)}
    for spec in specs:
        license_files = list(spec.get("license_files", [spec["license_file"]]))
        all_files = list(spec["files"]) + license_files
        missing = [p for p in all_files if not (ROOT / p).is_file()]
        fr, vr = functional.get(spec["name"], {}), verified.get(spec["name"], {})
        qos = [{"parameters": c.get("parameters", {}), "cells": c.get("qor", {}).get("cells"), "wires": c.get("qor", {}).get("wires"), "wire_bits": c.get("qor", {}).get("wire_bits")} for c in fr.get("cases", []) if isinstance(c, dict)]
        entries.append({"name": spec["name"], "module": spec["module"], "implementation": spec["implementation"], "source": spec["source"], "provider": "github", "status": "accepted", "family": spec["family"], "wrapper": spec["wrapper"], "files": spec["files"], "provenance": spec["provenance"], "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]}, "oracle": spec["oracle"], "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": [p for p in spec["files"] if p != spec["wrapper"]], "license_files": license_files, "include_roots": _include_roots(spec["files"])}, "validation": {"status": "passed" if fr.get("status") == "passed" else ("pending" if not fr else "failed"), "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.6.json", "configs": spec["configs"], "qor": qos, "functional_report": str(args.functional_report.relative_to(REPO).as_posix()) if args.functional_report.is_absolute() else str(args.functional_report), "runtime_gate_report": str(args.verify_report.relative_to(REPO).as_posix()) if args.verify_report.is_absolute() else str(args.verify_report), "runtime_gate": vr.get("status", "pending")}, "verification": vr})
    entries.sort(key=lambda e: e["name"])
    old["entries"] = entries; old["runtime_api_version"] = "0.6"; old["generated_by"] = "acir-runtime-promote-v06"
    CATALOG.write_text(json.dumps(old, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    hashes: dict[str, str] = {}
    for entry in entries:
        if entry.get("status") == "accepted":
            for item in list(entry.get("files", [])) + list(entry.get("dependency_closure", {}).get("license_files", [])):
                path = ROOT / item
                if path.is_file(): hashes[item] = _hash(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.6", "release": "runtime-rtl-v0.6", "generated_by": "acir-runtime-promote-v06", "toolchain": {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}, "policy": {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed", "cdc": "dual-clock components require an explicit clock-domain oracle"}, "components": [{"name": s["name"], "oracle": s["oracle"], "configs": s["configs"]} for s in specs], "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog": str(CATALOG), "manifest": str(MANIFEST), "entries": len(entries), "new_components": len(specs), "functional_pass": sum(functional.get(s["name"], {}).get("status") == "passed" for s in specs)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
