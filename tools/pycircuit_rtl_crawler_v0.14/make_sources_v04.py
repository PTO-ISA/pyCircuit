#!/usr/bin/env python3
"""Build the v0.4 source manifest with compilation-context metadata.

The v0.3 manifest intentionally described discovery roots only.  v0.4 keeps
that file immutable and records the additional include/vendor roots and
transitive repositories needed to elaborate a candidate as a standalone RTL
unit.  Dependency-only sources use ``scan: false``: they are materialised and
available to the closure builder but do not create duplicate discovery hits.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT_UPDATES = {
    "basejump_stl": {
        "yosys_overrides": {
            # CAM/gatestack variants elaborate much larger than ordinary
            # combinational cells. Keep them on a bounded structural path
            # with a longer deterministic budget.
            "bsg_cam_1r1w_sync_unmanaged": {"timeout_sec": 300, "frontend": "verilog", "skip_abc": True},
            "bsg_cam_1r1w_tag_array": {"timeout_sec": 300, "frontend": "verilog", "skip_abc": True},
            "bsg_cam_1r1w_unmanaged": {"timeout_sec": 300, "frontend": "verilog", "skip_abc": True},
            "bsg_fifo_1r1w_narrowed": {"timeout_sec": 300, "frontend": "verilog", "skip_abc": True},
            "bsg_mux_bitwise": {"timeout_sec": 180, "frontend": "verilog", "skip_abc": True},
            "bsg_mux_segmented": {"timeout_sec": 300, "frontend": "verilog", "skip_abc": True},
            "bsg_mux2_gatestack": {"timeout_sec": 300, "frontend": "verilog", "skip_abc": True},
            "bsg_muxi2_gatestack": {"timeout_sec": 300, "frontend": "verilog", "skip_abc": True},
        },
    },
    "opentitan": {
        "include_hints": ["hw/ip/prim_generic/rtl"],
        "dependency_projects": ["ibex"],
        "module_overrides": {
            "prim_and2": "ibex:vendor/lowrisc_ip/ip/prim_generic/rtl/prim_and2.sv",
        },
    },
    "pulp_axi": {
        "include_hints": ["include"],
        "dependency_projects": ["pulp_common_cells"],
        "prune_packages": ["assert_rpt_pkg", "uvm_pkg"],
        # The endpoint and interface variants are generic over AXI struct
        # types.  Give the validator a concrete typed context rather than
        # changing the upstream implementation.
        "validation_wrappers": {
            "axi_cdc_dst": {"kind": "axi4_endpoint", "log_depth": 1, "sync_stages": 3},
            "axi_cdc_src": {"kind": "axi4_endpoint", "log_depth": 1, "sync_stages": 3},
            "axi_cdc_intf": {"kind": "axi_cdc_intf", "log_depth": 1, "sync_stages": 3},
            "axi_lite_cdc_intf": {"kind": "axi_cdc_intf", "log_depth": 1, "sync_stages": 3},
            "axi_cdc_dst_intf": {"kind": "axi_cdc_intf", "log_depth": 1, "sync_stages": 3},
            "axi_lite_cdc_dst_intf": {"kind": "axi_cdc_intf", "log_depth": 1, "sync_stages": 3},
            "axi_cdc_src_intf": {"kind": "axi_cdc_intf", "log_depth": 1, "sync_stages": 3},
            "axi_lite_cdc_src_intf": {"kind": "axi_cdc_intf", "log_depth": 1, "sync_stages": 3},
        },
        "parameter_overrides": {
            "axi_cdc": {"LogDepth": 1, "SyncStages": 3},
            "axi_cdc_dst": {"LogDepth": 1, "SyncStages": 3},
            "axi_cdc_src": {"LogDepth": 1, "SyncStages": 3},
        },
    },
    "ibex": {
        "vendor_hints": ["vendor/lowrisc_ip/ip/prim", "vendor/lowrisc_ip/ip/prim_generic/rtl"],
        "exclude_dirs": [".git", "doc", "dv", "build"],
    },
    "cva6": {
        "include_hints": ["core/pmp/include", "core/include"],
        "dependency_projects": ["pulp_common_cells", "cva6_cvfpu"],
        "module_overrides": {
            "unread": "pulp_common_cells:src/deprecated/unread.sv",
        },
        "package_overrides": {
            "riscv": "cva6:core/include/riscv_pkg.sv",
            "cva6_config_pkg": "cva6:core/include/cv64a6_imafdc_sv39_config_pkg.sv",
        },
        "yosys_overrides": {
            "*": {"timeout_sec": 240, "frontend": "slang", "skip_abc": True}
        },
    },
    "vortex": {
        # VX_config.vh/VX_types.vh are generated below the repository's hw/
        # directory from VX_*toml; ci/ is needed by the deterministic generator.
        "include_hints": ["hw", "ci"],
        # VX_config.vh computes these defaults from localparams declared later
        # in VX_gpu_pkg.sv.  Supplying the fixed one-cluster/one-request
        # context removes that order-dependent elaboration cycle while
        # retaining the Vortex package and RTL unchanged.
        "defines": [
            "VX_CFG_XLEN=32", "VX_CFG_XLEN_32",
            "DCACHE_NUM_REQS=1", "L2_NUM_REQS=1", "L3_NUM_REQS=1",
            "VX_CFG_DCACHE_NUM_BANKS=1",
            "VX_CFG_L2_NUM_BANKS=1", "VX_CFG_L2_MEM_PORTS=1",
            "VX_CFG_L3_NUM_BANKS=1", "VX_CFG_L3_MEM_PORTS=1",
        ],
        "yosys_overrides": {
            "*": {
                "timeout_sec": 240,
                "frontend": "slang",
                "skip_abc": True,
                "allow_use_before_declare": True,
                "max_generate_steps": 20000,
            }
        },
    },
    "ohwr_general_cores": {
        "include_hints": ["modules/wishbone/wb_lm32/platform/generic"],
        "module_overrides": {
            "jtag_tap": "ohwr_general_cores:modules/wishbone/wb_lm32/platform/generic/jtag_tap.v",
            "lm32_multiplier": "ohwr_general_cores:modules/wishbone/wb_lm32/platform/generic/lm32_multiplier.v",
        },
    },
    "openpiton": {
        "include_hints": ["piton/design/include", "piton/design/chip/tile/common"],
        "yosys_overrides": {
            "*": {"timeout_sec": 180, "frontend": "slang", "skip_abc": True}
        },
    },
    "blackparrot": {
        "dependency_projects": ["basejump_stl", "bsg_hardfloat"],
    },
    "pulp_common_cells": {
        "dependency_projects": ["tech_cells_generic"],
        "prune_packages": ["assert_rpt_pkg", "uvm_pkg"],
        # Clearable CDC FIFOs require >=3 synchronizer stages when their
        # async-reset protocol is enabled.  This is a functional requirement
        # from the upstream source, not a black-box workaround.
        "parameter_overrides": {
            "cc_cdc_fifo_gray_clearable": {"SyncStages": 3, "LogDepth": 3, "ClearOnAsyncReset": 1},
            "cc_cdc_fifo_gray_clearable_src": {"SyncStages": 3, "LogDepth": 3},
            "cc_cdc_fifo_gray_clearable_dst": {"SyncStages": 3, "LogDepth": 3},
            "cc_cdc_fifo_gray_src_clearable": {"SyncStages": 3, "LogDepth": 3},
            "cc_cdc_fifo_gray_dst_clearable": {"SyncStages": 3, "LogDepth": 3},
            "cc_cdc_fifo_gray_src_ptr_clearable": {"SyncStages": 3, "LogDepth": 3},
            "cc_cdc_fifo_gray_dst_ptr_clearable": {"SyncStages": 3, "LogDepth": 3},
        },
        "yosys_overrides": {
            "*": {"timeout_sec": 180, "frontend": "slang", "skip_abc": True}
        },
    },
}

DEPENDENCY_SOURCES = [
    {
        "project": "cva6_cvfpu",
        "repo": "https://github.com/openhwgroup/cvfpu.git",
        "enabled": True,
        "scan": False,
        "priority": "D",
        "families": ["FloatingPoint"],
        "path_hints": ["src"],
        "exclude_dirs": [".git", "doc", "docs", "test", "tests", "formal", "build"],
        "extensions": [".v", ".sv", ".vh", ".svh"],
    },
    {
        "project": "tech_cells_generic",
        "repo": "https://github.com/pulp-platform/tech_cells_generic.git",
        "enabled": True,
        "scan": False,
        "priority": "D",
        "families": ["Technology", "CDC", "Clock", "Memory"],
        "path_hints": ["src"],
        "exclude_dirs": [".git", "docs", "test", "tests", "formal", "build"],
        "extensions": [".v", ".sv", ".vh", ".svh"],
    },
    {
        "project": "bsg_hardfloat",
        "repo": "https://github.com/bsg-external/HardFloat.git",
        "enabled": True,
        "scan": False,
        "priority": "D",
        "families": ["FloatingPoint"],
        "path_hints": ["source"],
        "exclude_dirs": [".git", "doc", "test", "tests", "build"],
        "extensions": [".v", ".sv", ".vh", ".svh", ".vi"],
    },
]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", default="sources.expanded.v0.3.json")
    ap.add_argument("--output", default="sources.expanded.v0.4.json")
    args = ap.parse_args()

    data = json.loads(Path(args.input).read_text(encoding="utf-8"))
    sources = [dict(s) for s in data.get("sources", [])]
    by_project = {s.get("project"): s for s in sources}
    for project, updates in ROOT_UPDATES.items():
        if project not in by_project:
            raise SystemExit(f"source project not found in input: {project}")
        item = by_project[project]
        for key, values in updates.items():
            if isinstance(values, list):
                old = list(item.get(key, []))
                item[key] = old + [x for x in values if x not in old]
            else:
                item[key] = values
    for dep in DEPENDENCY_SOURCES:
        if dep["project"] not in by_project:
            sources.append(dep)
    out = dict(data)
    out["schema"] = "pycircuit-rtl-sources-v0.4"
    out["notes"] = (
        "v0.4 adds include/vendor roots and explicit transitive dependency "
        "repositories. Dependency-only entries are materialized but not scanned. "
        "The validation context also records fixed per-module parameters, "
        "typed interface wrappers, project macro sets, and bounded Yosys "
        "frontend/timeout overrides; upstream RTL is never rewritten."
    )
    out["sources"] = sources
    Path(args.output).write_text(json.dumps(out, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {args.output} ({len(sources)} sources)")


if __name__ == "__main__":
    main()
