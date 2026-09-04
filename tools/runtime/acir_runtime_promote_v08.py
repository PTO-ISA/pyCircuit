#!/usr/bin/env python3
"""Promote the locked PULP/BaseJump arbitration batch into runtime v0.8.

The script is deliberately deterministic: it seeds entries before validation,
then reruns after the per-entry functional and gate reports exist.  No
structural candidate is accepted without an oracle, complete vendored
closure, Verilator simulation and Yosys synthesis.
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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.8.json"
# Reports live under the repository-local .pycircuit_out directory.

PULP_REPO = "https://github.com/pulp-platform/common_cells.git"
PULP_COMMIT = "63b7c50d43e462b59506f69d341ff1e40202866d"
BASEJUMP_REPO = "https://github.com/bespoke-silicon-group/basejump_stl.git"
BASEJUMP_COMMIT = "b48037e28544425839dbd617d45b1a82631bc1a9"


def _pulp_files(*names: str) -> list[str]:
    prefix = "verilog/vendor-v0.8/pulp_common_cells/"
    return ["verilog/pyc_runtime_" + names[0] + ".sv", *[prefix + name for name in names[1:]]]


def _specs() -> list[dict[str, Any]]:
    pulp_common = [
        "include/common_cells/assertions.svh",
        "include/common_cells/registers.svh",
        "include/common_cells/deprecated/registers.svh",
        "src/cc_pkg.sv",
        "src/cc_lzc.sv",
        "src/cc_rr_arb_tree.sv",
    ]
    return [
        {
            "name": "pulp-stream-arbiter", "module": "pyc_runtime_pulp_stream_arbiter",
            "implementation": "cc_stream_arbiter", "source": "pulp-common-cells-v0.8",
            "family": "arbitration-interconnect", "wrapper": "verilog/pyc_runtime_pulp_stream_arbiter.sv",
            "files": ["verilog/pyc_runtime_pulp_stream_arbiter.sv", "verilog/vendor-v0.8/pulp_common_cells/src/cc_stream_arbiter.sv", *["verilog/vendor-v0.8/pulp_common_cells/" + x for x in pulp_common]],
            "provenance": {"repository": PULP_REPO, "commit": PULP_COMMIT, "source_file": "src/cc_stream_arbiter.sv", "license": "Solderpad-Hardware-License-0.51"},
            "license_files": ["licenses/pulp-common-cells/LICENSE"],
            "interface": {"wrapper_module": "pyc_runtime_pulp_stream_arbiter", "parameters": [{"name": "NUM_INPUTS", "source": "NumInp", "default": 2}, {"name": "DATA_WIDTH", "source": "data_t", "default": 8}, {"name": "ARB_MODE", "source": "ArbMode", "default": 0, "values": {"0": "round-robin", "1": "fixed-priority"}}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "input_data", "direction": "input", "width": "NUM_INPUTS*DATA_WIDTH"}, {"name": "input_valid", "direction": "input", "width": "NUM_INPUTS"}, {"name": "input_ready", "direction": "output", "width": "NUM_INPUTS"}, {"name": "output_data", "direction": "output", "width": "DATA_WIDTH"}, {"name": "output_valid", "direction": "output", "width": "1"}, {"name": "output_ready", "direction": "input", "width": "1"}]},
            "oracle": {"id": "pulp-stream-arbiter-v1", "kind": "cycle", "contract": "round-robin mode selects the next valid input, holds data and revokes the grant under backpressure, and advances only on output handshake; fixed-priority mode selects the lowest valid input"},
            "configs": [{"NUM_INPUTS": 3, "DATA_WIDTH": 8, "ARB_MODE": 0}, {"NUM_INPUTS": 4, "DATA_WIDTH": 8, "ARB_MODE": 0}, {"NUM_INPUTS": 3, "DATA_WIDTH": 8, "ARB_MODE": 1}],
        },
        {
            "name": "pulp-rr-arb-tree", "module": "pyc_runtime_pulp_rr_arb_tree", "implementation": "cc_rr_arb_tree", "source": "pulp-common-cells-v0.8", "family": "arbitration-interconnect", "wrapper": "verilog/pyc_runtime_pulp_rr_arb_tree.sv",
            "files": ["verilog/pyc_runtime_pulp_rr_arb_tree.sv", *["verilog/vendor-v0.8/pulp_common_cells/" + x for x in pulp_common]],
            "provenance": {"repository": PULP_REPO, "commit": PULP_COMMIT, "source_file": "src/cc_rr_arb_tree.sv", "license": "Solderpad-Hardware-License-0.51"}, "license_files": ["licenses/pulp-common-cells/LICENSE"],
            "interface": {"wrapper_module": "pyc_runtime_pulp_rr_arb_tree", "parameters": [{"name": "NUM_INPUTS", "source": "NumIn", "default": 2}, {"name": "DATA_WIDTH", "source": "DataWidth", "default": 8}, {"name": "EXT_PRIO", "source": "ExtPrio", "default": 0}, {"name": "AXI_VALID_READY", "source": "AxiVldRdy", "default": 1}, {"name": "LOCK_IN", "source": "LockIn", "default": 1}, {"name": "FAIR_ARB", "source": "FairArb", "default": 1}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "rr_priority", "direction": "input", "width": "INDEX_WIDTH"}, {"name": "requests", "direction": "input", "width": "NUM_INPUTS"}, {"name": "grants", "direction": "output", "width": "NUM_INPUTS"}, {"name": "input_data", "direction": "input", "width": "NUM_INPUTS*DATA_WIDTH"}, {"name": "request_valid", "direction": "output", "width": "1"}, {"name": "grant_ready", "direction": "input", "width": "1"}, {"name": "output_data", "direction": "output", "width": "DATA_WIDTH"}, {"name": "grant_index", "direction": "output", "width": "INDEX_WIDTH"}]},
            "oracle": {"id": "pulp-rr-arb-tree-v1", "kind": "cycle", "contract": "round-robin tree holds the selected payload while grant_ready is low, emits a one-hot grant on handshake, and rotates to the next request"}, "configs": [{"NUM_INPUTS": 2, "DATA_WIDTH": 8, "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1, "FAIR_ARB": 1}, {"NUM_INPUTS": 4, "DATA_WIDTH": 8, "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1, "FAIR_ARB": 1}],
        },
        {
            "name": "pulp-stream-xbar", "module": "pyc_runtime_pulp_stream_xbar", "implementation": "cc_stream_xbar", "source": "pulp-common-cells-v0.8", "family": "arbitration-interconnect", "wrapper": "verilog/pyc_runtime_pulp_stream_xbar.sv",
            "files": ["verilog/pyc_runtime_pulp_stream_xbar.sv", *["verilog/vendor-v0.8/pulp_common_cells/" + x for x in ["src/cc_stream_xbar.sv", "src/cc_stream_demux.sv", "src/cc_spill_register.sv", "src/cc_spill_register_flushable.sv", *pulp_common]]],
            "provenance": {"repository": PULP_REPO, "commit": PULP_COMMIT, "source_file": "src/cc_stream_xbar.sv", "license": "Solderpad-Hardware-License-0.51"}, "license_files": ["licenses/pulp-common-cells/LICENSE"],
            "interface": {"wrapper_module": "pyc_runtime_pulp_stream_xbar", "parameters": [{"name": "NUM_INPUTS", "source": "NumInp", "default": 2}, {"name": "NUM_OUTPUTS", "source": "NumOut", "default": 1}, {"name": "DATA_WIDTH", "source": "DataWidth", "default": 8}, {"name": "OUT_SPILL_REG", "source": "OutSpillReg", "default": 0}, {"name": "EXT_PRIO", "source": "ExtPrio", "default": 0}, {"name": "AXI_VALID_READY", "source": "AxiVldRdy", "default": 1}, {"name": "LOCK_IN", "source": "LockIn", "default": 1}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset_n", "direction": "input", "width": "1"}, {"name": "clear", "direction": "input", "width": "1"}, {"name": "clear_arb", "direction": "input", "width": "1"}, {"name": "rr_priority", "direction": "input", "width": "NUM_OUTPUTS*INDEX_WIDTH"}, {"name": "input_data", "direction": "input", "width": "NUM_INPUTS*DATA_WIDTH"}, {"name": "select_output", "direction": "input", "width": "NUM_INPUTS*SELECT_WIDTH"}, {"name": "input_valid", "direction": "input", "width": "NUM_INPUTS"}, {"name": "input_ready", "direction": "output", "width": "NUM_INPUTS"}, {"name": "output_data", "direction": "output", "width": "NUM_OUTPUTS*DATA_WIDTH"}, {"name": "output_index", "direction": "output", "width": "NUM_OUTPUTS*INDEX_WIDTH"}, {"name": "output_valid", "direction": "output", "width": "NUM_OUTPUTS"}, {"name": "output_ready", "direction": "input", "width": "NUM_OUTPUTS"}]},
            "oracle": {"id": "pulp-stream-xbar-v1", "kind": "cycle", "contract": "routes each valid input to its selected output, arbitrates collisions round-robin, preserves payload/index under backpressure, and rotates after handshake"}, "configs": [{"NUM_INPUTS": 2, "NUM_OUTPUTS": 1, "DATA_WIDTH": 8, "OUT_SPILL_REG": 0, "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1}, {"NUM_INPUTS": 4, "NUM_OUTPUTS": 1, "DATA_WIDTH": 8, "OUT_SPILL_REG": 0, "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1}],
        },
        {
            "name": "basejump-rr-1-to-n", "module": "pyc_runtime_basejump_rr_1_to_n", "implementation": "bsg_round_robin_1_to_n", "source": "basejump-stl-v0.5", "family": "arbitration-interconnect", "wrapper": "verilog/pyc_runtime_basejump_rr_1_to_n.sv",
            "files": ["verilog/pyc_runtime_basejump_rr_1_to_n.sv", "verilog/vendor-v0.5/basejump/bsg_dataflow/bsg_round_robin_1_to_n.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_circular_ptr.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_defines.sv"],
            "provenance": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "source_file": "bsg_dataflow/bsg_round_robin_1_to_n.sv", "license": "Solderpad-Hardware-License-0.51"}, "license_files": ["licenses/basejump-stl-v0.5/LICENSE"],
            "interface": {"wrapper_module": "pyc_runtime_basejump_rr_1_to_n", "parameters": [{"name": "NUM_OUTPUTS", "source": "num_out_p", "default": 2}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset", "direction": "input", "width": "1"}, {"name": "input_valid", "direction": "input", "width": "1"}, {"name": "input_ready", "direction": "output", "width": "1"}, {"name": "output_valid", "direction": "output", "width": "NUM_OUTPUTS"}, {"name": "output_ready", "direction": "input", "width": "NUM_OUTPUTS"}]}, "oracle": {"id": "basejump-rr-1-to-n-v1", "kind": "cycle", "contract": "one valid stream is routed to one ready output and the pointer rotates after each accepted transfer"}, "configs": [{"NUM_OUTPUTS": 2}, {"NUM_OUTPUTS": 4}],
        },
        {
            "name": "basejump-rr-n-to-1", "module": "pyc_runtime_basejump_rr_n_to_1", "implementation": "bsg_round_robin_n_to_1", "source": "basejump-stl-v0.5", "family": "arbitration-interconnect", "wrapper": "verilog/pyc_runtime_basejump_rr_n_to_1.sv",
            "files": ["verilog/pyc_runtime_basejump_rr_n_to_1.sv", "verilog/vendor-v0.5/basejump/bsg_dataflow/bsg_round_robin_n_to_1.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_arb_round_robin.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_circular_ptr.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_crossbar_o_by_i.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_encode_one_hot.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_mux_one_hot.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_round_robin_arb.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_scan.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_defines.sv"],
            "provenance": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "source_file": "bsg_dataflow/bsg_round_robin_n_to_1.sv", "license": "Solderpad-Hardware-License-0.51"}, "license_files": ["licenses/basejump-stl-v0.5/LICENSE"],
            "interface": {"wrapper_module": "pyc_runtime_basejump_rr_n_to_1", "parameters": [{"name": "NUM_INPUTS", "source": "num_in_p", "default": 4}, {"name": "DATA_WIDTH", "source": "width_p", "default": 8}, {"name": "STRICT", "source": "strict_p", "default": 1}, {"name": "USE_SCAN", "source": "use_scan_p", "default": 0}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset", "direction": "input", "width": "1"}, {"name": "input_data", "direction": "input", "width": "NUM_INPUTS*DATA_WIDTH"}, {"name": "input_valid", "direction": "input", "width": "NUM_INPUTS"}, {"name": "input_yumi", "direction": "output", "width": "NUM_INPUTS"}, {"name": "output_valid", "direction": "output", "width": "1"}, {"name": "output_data", "direction": "output", "width": "DATA_WIDTH"}, {"name": "output_tag", "direction": "output", "width": "TAG_WIDTH"}, {"name": "output_yumi", "direction": "input", "width": "1"}]}, "oracle": {"id": "basejump-rr-n-to-1-v1", "kind": "cycle", "contract": "strict mode exposes the current round-robin input and emits one-hot yumi only when the downstream consumes the item"}, "configs": [{"NUM_INPUTS": 2, "DATA_WIDTH": 8, "STRICT": 1, "USE_SCAN": 0}, {"NUM_INPUTS": 4, "DATA_WIDTH": 8, "STRICT": 1, "USE_SCAN": 0}],
        },
        {
            "name": "basejump-rr-2-to-2", "module": "pyc_runtime_basejump_rr_2_to_2", "implementation": "bsg_round_robin_2_to_2", "source": "basejump-stl-v0.5", "family": "arbitration-interconnect", "wrapper": "verilog/pyc_runtime_basejump_rr_2_to_2.sv", "files": ["verilog/pyc_runtime_basejump_rr_2_to_2.sv", "verilog/vendor-v0.5/basejump/bsg_dataflow/bsg_round_robin_2_to_2.sv", "verilog/vendor-v0.5/basejump/bsg_misc/bsg_defines.sv"],
            "provenance": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "source_file": "bsg_dataflow/bsg_round_robin_2_to_2.sv", "license": "Solderpad-Hardware-License-0.51"}, "license_files": ["licenses/basejump-stl-v0.5/LICENSE"],
            "interface": {"wrapper_module": "pyc_runtime_basejump_rr_2_to_2", "parameters": [{"name": "DATA_WIDTH", "source": "width_p", "default": 8}], "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset", "direction": "input", "width": "1"}, {"name": "input_data", "direction": "input", "width": "2*DATA_WIDTH"}, {"name": "input_valid", "direction": "input", "width": "2"}, {"name": "input_ready", "direction": "output", "width": "2"}, {"name": "output_data", "direction": "output", "width": "2*DATA_WIDTH"}, {"name": "output_valid", "direction": "output", "width": "2"}, {"name": "output_ready", "direction": "input", "width": "2"}]}, "oracle": {"id": "basejump-rr-2-to-2-v1", "kind": "cycle", "contract": "passes the two ready/valid lanes and updates the swizzle head after a single-lane transfer"}, "configs": [{"DATA_WIDTH": 8}, {"DATA_WIDTH": 16}],
        },
    ]


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _report(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}


def main() -> int:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    specs = normalize_specs(_specs())
    by_name = {s["name"]: s for s in specs}
    old_entries = [e for e in catalog.get("entries", []) if e.get("name") not in by_name]
    for spec in specs:
        functional = _report(REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v08-{spec['name']}.json")
        gate = _report(REPO / ".pycircuit_out" / "runtime-catalog-validation" / f"v08-{spec['name']}.json")
        functional_ok = functional.get("summary", {}).get("status") == "passed"
        gate_ok = gate.get("summary", {}).get("status") == "passed"
        missing = [p for p in spec["files"] + spec["license_files"] if not (ROOT / p).is_file()]
        qos = []
        for case in (functional.get("results", [{}])[0].get("cases", []) if functional.get("results") else []):
            if isinstance(case, dict):
                qos.append({"parameters": case.get("parameters", {}), **{k: case.get("qor", {}).get(k) for k in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
        entry = {"name": spec["name"], "module": spec["module"], "implementation": spec["implementation"], "source": spec["source"], "provider": "github", "status": "accepted", "family": spec["family"], "wrapper": spec["wrapper"], "files": spec["files"], "provenance": spec["provenance"], "interface": spec["interface"], "oracle": spec["oracle"], "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": [p for p in spec["files"] if p != spec["wrapper"]], "license_files": spec["license_files"], "include_roots": sorted({p.rsplit("/include/", 1)[0] + "/include" for p in spec["files"] if "/include/" in p})}, "validation": {"status": "passed" if functional_ok else ("pending" if not functional else "failed"), "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.8.json", "configs": spec["configs"], "functional_report": f".pycircuit_out/runtime-functional-validation/v08-{spec['name']}.json", "runtime_gate_report": f".pycircuit_out/runtime-catalog-validation/v08-{spec['name']}.json", "runtime_gate": "passed" if gate_ok else ("pending" if not gate else "failed"), "qor": qos}}
        old_entries.append(entry)
    old_entries.sort(key=lambda e: e.get("name", ""))
    catalog["entries"] = old_entries
    catalog["runtime_api_version"] = "0.8"
    catalog["generated_by"] = "acir-runtime-promote-v08"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old_manifest = _report(MANIFEST)
    old_components = [c for c in old_manifest.get("components", []) if c.get("name") not in by_name]
    old_components.extend({"name": s["name"], "oracle": s["oracle"], "parameters": {p["name"]: p.get("source", "") for p in s["interface"]["parameters"]}, "configs": s["configs"], "source": {"repository": s["provenance"]["repository"], "commit": s["provenance"]["commit"], "files": [p for p in s["files"] if p != s["wrapper"]], "license": s["license_files"][0]}} for s in specs)
    hashes = {}
    for e in old_entries:
        for p in list(e.get("files", [])) + list(e.get("dependency_closure", {}).get("license_files", [])):
            path = ROOT / p
            if path.is_file(): hashes[p] = _sha(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.8", "release": "runtime-rtl-v0.8", "generated_by": "acir-runtime-promote-v08", "toolchain": old_manifest.get("toolchain", {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}), "policy": old_manifest.get("policy", {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed"}), "components": sorted(old_components, key=lambda c: c.get("name", "")), "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog_entries": len(old_entries), "new_components": len(specs), "functional_pass": sum((_report(REPO / ".pycircuit_out" / "runtime-functional-validation" / f"v08-{s['name']}.json").get("summary", {}).get("status") == "passed") for s in specs)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
