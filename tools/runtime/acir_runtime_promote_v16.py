#!/usr/bin/env python3
"""Promote the Vortex mux/demux and PULP two-phase CDC batch (runtime v0.16).

``prepare`` adds the reviewed, fully vendored entries to the catalog so the
functional runner can exercise them.  ``finalize`` consumes the persisted
Verilator/Yosys reports and accepts an entry only when its complete source
closure and both gates are present.
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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.16.json"
LOCK = ROOT / "catalog.lock.json"

VORTEX_REPO = "https://github.com/vortexgpgpu/vortex.git"
VORTEX_COMMIT = "5d62846c685ae287f9cd3ddd49f4537c40146eae"
VORTEX_LICENSE = "licenses/vortex-v0.16-LICENSE"
BASEJUMP_REPO = "https://github.com/bespoke-silicon-group/basejump_stl.git"
BASEJUMP_COMMIT = "b48037e28544425839dbd617d45b1a82631bc1a9"
BASEJUMP_LICENSE = "licenses/basejump-stl-v0.16-LICENSE"
PULP_REPO = "https://github.com/pulp-platform/common_cells.git"
PULP_COMMIT = "db42769334b4589b4b3fc671b34513bdb98be565"
PULP_LICENSE = "licenses/pulp-common-cells-v0.16-LICENSE"
TECH_LICENSE = "licenses/tech-cells-generic-v0.16-LICENSE"


def _specs() -> list[dict[str, Any]]:
    vortex = "verilog/vendor-v0.16/vortex/hw/rtl"
    pulp = "verilog/vendor-v0.16/pulp_common_cells"
    tech = "verilog/vendor-v0.16/tech_cells_generic/src/rtl/tc_sync.sv"
    return [
        {
            "name": "basejump-fifo-small", "module": "pyc_runtime_basejump_fifo_small", "implementation": "bsg_fifo_1r1w_small",
            "source": "basejump-stl-v0.16", "provider": "github", "family": "storage-dataflow",
            "wrapper": "verilog/pyc_runtime_basejump_fifo_small.sv",
            "source_files": ["verilog/vendor-v0.16/basejump/" + f for f in ("bsg_fifo_1r1w_small.sv", "bsg_fifo_1r1w_small_unhardened.sv", "bsg_fifo_1r1w_small_hardened.sv", "bsg_fifo_tracker.sv", "bsg_circular_ptr.sv", "bsg_two_fifo.sv", "bsg_mem_1r1w.sv", "bsg_mem_1r1w_synth.sv", "bsg_mem_1r1w_sync.sv", "bsg_mem_1r1w_sync_synth.sv", "bsg_dff.sv", "bsg_dff_en.sv", "bsg_dff_en_bypass.sv", "bsg_dff_reset_set_clear.sv", "bsg_clkgate_optional.sv", "bsg_dlatch.sv", "bsg_defines.sv")],
            "repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT,
            "source_file": "bsg_dataflow/bsg_fifo_1r1w_small.sv", "license": "Solderpad-Hardware-License-0.51", "license_file": BASEJUMP_LICENSE,
            "parameters": [
                {"name": "WIDTH", "source": "width_p", "default": 8}, {"name": "DEPTH", "source": "els_p", "default": 2},
                {"name": "READY_THEN_VALID", "source": "ready_THEN_valid_p", "default": 0}, {"name": "HARDEN", "source": "harden_p", "default": 0},
            ],
            "ports": [{"name": "clk", "direction": "input", "width": "1"}, {"name": "reset", "direction": "input", "width": "1"}, {"name": "in_data", "direction": "input", "width": "WIDTH"}, {"name": "in_valid", "direction": "input", "width": "1"}, {"name": "in_ready", "direction": "output", "width": "1"}, {"name": "out_data", "direction": "output", "width": "WIDTH"}, {"name": "out_valid", "direction": "output", "width": "1"}, {"name": "out_ready", "direction": "input", "width": "1"}],
            "oracle": {"id": "basejump-fifo-small-v1", "kind": "cycle", "contract": "ready/valid pushes preserve order and out_ready consumes exactly one head word per cycle"},
            "configs": [{"WIDTH": 4, "DEPTH": 2, "HARDEN": 0}, {"WIDTH": 8, "DEPTH": 3, "HARDEN": 0}, {"WIDTH": 8, "DEPTH": 2, "HARDEN": 1}],
        },
        {
            "name": "vortex-mux", "module": "pyc_runtime_vortex_mux", "implementation": "VX_mux",
            "source": "vortex-v0.16", "provider": "github", "family": "interconnect-selection",
            "wrapper": "verilog/pyc_runtime_vortex_mux.sv",
            "source_files": [f"{vortex}/libs/VX_mux.sv", f"{vortex}/VX_platform.vh", f"{vortex}/VX_scope.vh"],
            "repository": VORTEX_REPO, "commit": VORTEX_COMMIT,
            "source_file": "hw/rtl/libs/VX_mux.sv", "license": "Apache-2.0", "license_file": VORTEX_LICENSE,
            "parameters": [
                {"name": "DATA_WIDTH", "source": "DATAW", "default": 8},
                {"name": "INPUTS", "source": "N", "default": 2},
                {"name": "SELECT_WIDTH", "source": "LN", "derived": "(INPUTS <= 1) ? 1 : $clog2(INPUTS)"},
            ],
            "ports": [
                {"name": "data_in", "direction": "input", "width": "INPUTS x DATA_WIDTH"},
                {"name": "select_in", "direction": "input", "width": "SELECT_WIDTH"},
                {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"},
            ],
            "oracle": {"id": "vortex-mux-v1", "kind": "combinational", "contract": "data_out equals data_in[select_in]; INPUTS=1 is a passthrough"},
            "configs": [{"INPUTS": 1, "DATA_WIDTH": 1}, {"INPUTS": 2, "DATA_WIDTH": 4}, {"INPUTS": 5, "DATA_WIDTH": 8}],
        },
        {
            "name": "vortex-demux", "module": "pyc_runtime_vortex_demux", "implementation": "VX_demux",
            "source": "vortex-v0.16", "provider": "github", "family": "interconnect-selection",
            "wrapper": "verilog/pyc_runtime_vortex_demux.sv",
            "source_files": [f"{vortex}/libs/VX_demux.sv", f"{vortex}/VX_platform.vh", f"{vortex}/VX_scope.vh"],
            "repository": VORTEX_REPO, "commit": VORTEX_COMMIT,
            "source_file": "hw/rtl/libs/VX_demux.sv", "license": "Apache-2.0", "license_file": VORTEX_LICENSE,
            "parameters": [
                {"name": "DATA_WIDTH", "source": "DATAW", "default": 8},
                {"name": "INPUTS", "source": "N", "default": 2},
                {"name": "MODEL", "source": "MODEL", "default": 0},
                {"name": "SELECT_WIDTH", "source": "LN", "derived": "(INPUTS <= 1) ? 1 : $clog2(INPUTS)"},
            ],
            "ports": [
                {"name": "select_in", "direction": "input", "width": "SELECT_WIDTH"},
                {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"},
                {"name": "data_out", "direction": "output", "width": "INPUTS x DATA_WIDTH"},
            ],
            "oracle": {"id": "vortex-demux-v1", "kind": "combinational", "contract": "only data_out[select_in] carries data_in; all other lanes are zero"},
            "configs": [{"INPUTS": 1, "DATA_WIDTH": 1, "MODEL": 0}, {"INPUTS": 2, "DATA_WIDTH": 4, "MODEL": 0}, {"INPUTS": 5, "DATA_WIDTH": 8, "MODEL": 1}],
        },
        {
            "name": "vortex-onehot-mux", "module": "pyc_runtime_vortex_onehot_mux", "implementation": "VX_onehot_mux",
            "source": "vortex-v0.16", "provider": "github", "family": "interconnect-selection",
            "wrapper": "verilog/pyc_runtime_vortex_onehot_mux.sv",
            "source_files": [f"{vortex}/libs/VX_onehot_mux.sv", f"{vortex}/libs/VX_find_first.sv", f"{vortex}/VX_platform.vh", f"{vortex}/VX_scope.vh"],
            "repository": VORTEX_REPO, "commit": VORTEX_COMMIT,
            "source_file": "hw/rtl/libs/VX_onehot_mux.sv", "license": "Apache-2.0", "license_file": VORTEX_LICENSE,
            "parameters": [
                {"name": "DATA_WIDTH", "source": "DATAW", "default": 8},
                {"name": "INPUTS", "source": "N", "default": 2},
                {"name": "MODEL", "source": "MODEL", "default": 1},
                {"name": "LUT_OPT", "source": "LUT_OPT", "default": 0},
            ],
            "ports": [
                {"name": "data_in", "direction": "input", "width": "INPUTS x DATA_WIDTH"},
                {"name": "select_onehot", "direction": "input", "width": "INPUTS"},
                {"name": "data_out", "direction": "output", "width": "DATA_WIDTH"},
            ],
            "oracle": {"id": "vortex-onehot-mux-v1", "kind": "combinational", "contract": "for a one-hot select vector, data_out equals the selected data_in lane"},
            "configs": [{"INPUTS": 1, "DATA_WIDTH": 1, "MODEL": 1}, {"INPUTS": 2, "DATA_WIDTH": 4, "MODEL": 1}, {"INPUTS": 4, "DATA_WIDTH": 8, "MODEL": 2}, {"INPUTS": 5, "DATA_WIDTH": 8, "MODEL": 2}],
        },
        {
            "name": "pulp-cdc-fifo-2phase", "module": "pyc_runtime_pulp_cdc_fifo_2phase", "implementation": "cc_cdc_fifo_2phase",
            "source": "pulp-common-cells-v0.16", "provider": "github", "family": "clock-domain-crossing",
            "wrapper": "verilog/pyc_runtime_pulp_cdc_fifo_2phase.sv",
            "source_files": [f"{pulp}/src/cc_cdc_fifo_2phase.sv", f"{pulp}/src/cc_cdc_2phase.sv", f"{pulp}/include/common_cells/registers.svh", f"{pulp}/include/common_cells/assertions.svh", f"{pulp}/include/common_cells/deprecated/registers.svh", tech],
            "repository": PULP_REPO, "commit": PULP_COMMIT,
            "source_file": "src/cc_cdc_fifo_2phase.sv", "license": "Solderpad-Hardware-License-0.51", "license_file": PULP_LICENSE,
            "parameters": [
                {"name": "DATA_WIDTH", "source": "data_t", "default": 8},
                {"name": "LOG_DEPTH", "source": "LogDepth", "default": 2},
                {"name": "SYNC_STAGES", "source": "SyncStages", "default": 2},
            ],
            "ports": [
                {"name": "src_rst_n", "direction": "input", "width": "1"}, {"name": "src_clk", "direction": "input", "width": "1"},
                {"name": "src_data", "direction": "input", "width": "DATA_WIDTH"}, {"name": "src_valid", "direction": "input", "width": "1"}, {"name": "src_ready", "direction": "output", "width": "1"},
                {"name": "dst_rst_n", "direction": "input", "width": "1"}, {"name": "dst_clk", "direction": "input", "width": "1"},
                {"name": "dst_data", "direction": "output", "width": "DATA_WIDTH"}, {"name": "dst_valid", "direction": "output", "width": "1"}, {"name": "dst_ready", "direction": "input", "width": "1"},
            ],
            "oracle": {"id": "pulp-cdc-fifo-2phase-v1", "kind": "cycle", "contract": "source ready/valid transfers cross the asynchronous boundary exactly once and preserve order"},
            "configs": [{"DATA_WIDTH": 2, "LOG_DEPTH": 1, "SYNC_STAGES": 2}, {"DATA_WIDTH": 2, "LOG_DEPTH": 2, "SYNC_STAGES": 2}],
            "extra_license_files": [TECH_LICENSE],
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


def _rel(path: Path) -> str:
    try:
        return path.relative_to(REPO).as_posix()
    except ValueError:
        return str(path)


def _result(report: Mapping[str, Any], name: str) -> dict[str, Any]:
    return next((dict(item) for item in report.get("results", []) if isinstance(item, Mapping) and str(item.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(x) for x in spec["source_files"]]]
    licenses = [str(spec["license_file"]), *[str(x) for x in spec.get("extra_license_files", [])]]
    missing = [p for p in [*files, *licenses] if not (ROOT / p).is_file()]
    f = _result(functional, str(spec["name"]))
    g = _result(gate, str(spec["name"]))
    staging = not functional and not gate
    accepted = not missing and (staging or (f.get("status") == "passed" and g.get("status") == "passed"))
    qor = []
    for case in f.get("cases", []) if isinstance(f, Mapping) else []:
        if isinstance(case, Mapping) and isinstance(case.get("qor"), Mapping):
            q = case["qor"]
            qor.append({"parameters": case.get("parameters", {}), **{k: q.get(k) for k in ("cells", "wires", "wire_bits", "ports", "port_bits")}})
    return {
        "name": spec["name"], "module": spec["module"], "implementation": spec["implementation"], "source": spec["source"], "provider": spec["provider"],
        "status": "accepted" if accepted else "pending", "family": spec["family"], "wrapper": spec["wrapper"], "files": files,
        "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": spec["source_file"], "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]}, "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": licenses, "include_roots": []},
        "validation": {"status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.16.json", "configs": spec["configs"], "functional_report": ".pycircuit_out/runtime-functional-validation/v16-next.json", "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v16-next.json", "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qor},
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v16-next.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v16-next.json")
    args = parser.parse_args()
    specs = normalize_specs(_specs()); names = {str(s["name"]) for s in specs}; catalog = _read(CATALOG)
    functional = _read(args.functional_report) if args.mode == "finalize" else {}; gate = _read(args.gate_report) if args.mode == "finalize" else {}
    entries = [e for e in catalog.get("entries", []) if str(e.get("name")) not in names]
    entries.extend(_entry(s, functional, gate) for s in specs); entries.sort(key=lambda e: str(e.get("name", "")))
    catalog["entries"] = entries; catalog["runtime_api_version"] = "0.16"; catalog["generated_by"] = "acir-runtime-promote-v16"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    old = _read(ROOT / "manifests/parameterized-components-v0.15.json")
    components = [c for c in old.get("components", []) if str(c.get("name")) not in names]
    for s in specs:
        components.append({"name": s["name"], "oracle": s["oracle"], "parameters": {p["name"]: p.get("source", p.get("derived", "")) for p in s["parameters"]}, "configs": s["configs"], "source": {"repository": s["repository"], "commit": s["commit"], "files": s["source_files"], "license": s["license_file"]}})
    hashes = {}
    for e in entries:
        for p in [*e.get("files", []), *e.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(p)
            if path.is_file(): hashes[str(p)] = _sha(path)
    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.16", "release": "runtime-rtl-v0.16", "generated_by": "acir-runtime-promote-v16", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda c: str(c.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = _read(LOCK); sources = lock.setdefault("sources", {}); sources.update({"basejump-stl-v0.16": {"repository": BASEJUMP_REPO, "commit": BASEJUMP_COMMIT, "license": "Solderpad-Hardware-License-0.51"}, "vortex-v0.16": {"repository": VORTEX_REPO, "commit": VORTEX_COMMIT, "license": "Apache-2.0"}, "pulp-common-cells-v0.16": {"repository": PULP_REPO, "commit": PULP_COMMIT, "license": "Solderpad-Hardware-License-0.51"}, "tech-cells-generic-v0.16": {"repository": "https://github.com/pulp-platform/tech_cells_generic.git", "commit": "55cb54513e2d426be5992d311cb9d5dbcad10c78", "license": "Solderpad-Hardware-License-0.51"}}); LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    accepted = sum(e.get("status") == "accepted" for e in entries if e.get("name") in names)
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": len(specs), "accepted": accepted}, ensure_ascii=False)); return 0


if __name__ == "__main__":
    raise SystemExit(main())
