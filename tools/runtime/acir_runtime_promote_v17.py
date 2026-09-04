#!/usr/bin/env python3
"""Promote the PULP clearable gray-pointer CDC FIFO (runtime v0.17).

The release is report-driven: ``prepare`` stages the reviewed wrapper and
complete source closure, while ``finalize`` records the functional and
packaged Verilator/Yosys evidence in the catalog and release manifest.
"""

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
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.17.json"
LOCK = ROOT / "catalog.lock.json"

PULP_REPO = "https://github.com/pulp-platform/common_cells.git"
PULP_COMMIT = "db42769334b4589b4b3fc671b34513bdb98be565"
PULP_LICENSE = "licenses/pulp-common-cells-v0.17-LICENSE"
TECH_REPO = "https://github.com/pulp-platform/tech_cells_generic.git"
TECH_COMMIT = "55cb54513e2d426be5992d311cb9d5dbcad10c78"
TECH_LICENSE = "licenses/tech-cells-generic-v0.16-LICENSE"


def _spec() -> dict[str, Any]:
    v = "verilog/vendor-v0.17"
    pulp = f"{v}/pulp_common_cells"
    name = "pulp-cdc-fifo-gray-clearable"
    source_files = [
        f"{pulp}/src/cc_cdc_fifo_gray_clearable.sv",
        f"{pulp}/src/cc_cdc_reset_ctrlr.sv",
        f"{pulp}/src/cc_cdc_4phase.sv",
        f"{pulp}/src/cc_pkg.sv",
        f"{pulp}/src/cc_binary_to_gray.sv",
        f"{pulp}/src/cc_gray_to_binary.sv",
        f"{pulp}/src/cc_spill_register.sv",
        f"{pulp}/src/cc_spill_register_flushable.sv",
        f"{pulp}/include/common_cells/registers.svh",
        f"{pulp}/include/common_cells/assertions.svh",
        f"{pulp}/include/common_cells/deprecated/registers.svh",
        f"{v}/tech_cells_generic/src/rtl/tc_sync.sv",
    ]
    return {
        "name": name,
        "module": "pyc_runtime_pulp_cdc_fifo_gray_clearable",
        "implementation": "cc_cdc_fifo_gray_clearable",
        "source": "pulp-common-cells-v0.17",
        "provider": "github",
        "family": "clock-domain-crossing",
        "wrapper": "verilog/pyc_runtime_pulp_cdc_fifo_gray_clearable.sv",
        "source_files": source_files,
        "repository": PULP_REPO,
        "commit": PULP_COMMIT,
        "source_file": "src/cc_cdc_fifo_gray_clearable.sv",
        "license": "Solderpad-Hardware-License-0.51",
        "license_file": PULP_LICENSE,
        "extra_license_files": [TECH_LICENSE],
        "parameters": [
            {"name": "DATA_WIDTH", "source": "Width", "default": 8},
            {"name": "LOG_DEPTH", "source": "LogDepth", "default": 2},
            {"name": "SYNC_STAGES", "source": "SyncStages", "default": 3},
            {"name": "CLEAR_ON_ASYNC_RESET", "source": "ClearOnAsyncReset", "default": 1},
        ],
        "ports": [
            {"name": "src_rst_n", "direction": "input", "width": "1"},
            {"name": "src_clk", "direction": "input", "width": "1"},
            {"name": "src_clear", "direction": "input", "width": "1"},
            {"name": "src_clear_pending", "direction": "output", "width": "1"},
            {"name": "src_data", "direction": "input", "width": "DATA_WIDTH"},
            {"name": "src_valid", "direction": "input", "width": "1"},
            {"name": "src_ready", "direction": "output", "width": "1"},
            {"name": "dst_rst_n", "direction": "input", "width": "1"},
            {"name": "dst_clk", "direction": "input", "width": "1"},
            {"name": "dst_clear", "direction": "input", "width": "1"},
            {"name": "dst_clear_pending", "direction": "output", "width": "1"},
            {"name": "dst_data", "direction": "output", "width": "DATA_WIDTH"},
            {"name": "dst_valid", "direction": "output", "width": "1"},
            {"name": "dst_ready", "direction": "input", "width": "1"},
        ],
        "oracle": {"id": "pulp-cdc-fifo-gray-clearable-v1", "kind": "cycle/clock-domain-crossing", "contract": "a source or destination clear isolates both domains, drops pre-clear contents without duplication, then returns both sides to ready/valid operation"},
        "configs": [
            {"DATA_WIDTH": 8, "LOG_DEPTH": 2, "SYNC_STAGES": 2, "CLEAR_ON_ASYNC_RESET": 0},
            # SyncStages=3 with LogDepth=2 is functionally legal but causes
            # the upstream source to emit a $warning (2*SyncStages > depth),
            # which Yosys' Slang frontend cannot lower.  LogDepth=3 is the
            # next legal CDC configuration and exercises the same clear-on-
            # asynchronous-reset contract without hiding diagnostics.
            {"DATA_WIDTH": 8, "LOG_DEPTH": 3, "SYNC_STAGES": 3, "CLEAR_ON_ASYNC_RESET": 1},
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


def _relative(path: Path) -> str:
    try:
        return path.relative_to(REPO).as_posix()
    except ValueError:
        return str(path)


def _result(report: Mapping[str, Any], name: str) -> dict[str, Any]:
    return next((dict(x) for x in report.get("results", []) if isinstance(x, Mapping) and str(x.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(x) for x in spec["source_files"]]]
    licenses = [str(spec["license_file"]), *[str(x) for x in spec.get("extra_license_files", [])]]
    missing = [p for p in [*files, *licenses] if not (ROOT / p).is_file()]
    f = _result(functional, str(spec["name"]))
    g = _result(gate, str(spec["name"]))
    staging = not functional and not gate
    accepted = not missing and (staging or (f.get("status") == "passed" and g.get("status") == "passed"))
    qor: list[dict[str, Any]] = []
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
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": licenses, "include_roots": []},
        "validation": {"status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.17.json", "configs": spec["configs"], "functional_report": _relative(REPO / ".pycircuit_out/runtime-functional-validation/v17-clearable.json"), "runtime_gate_report": _relative(REPO / ".pycircuit_out/runtime-catalog-validation/v17-clearable.json"), "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qor},
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v17-clearable.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v17-clearable.json")
    args = parser.parse_args()
    spec = normalize_spec(_spec()); name = str(spec["name"]); catalog = _read(CATALOG)
    functional = _read(args.functional_report) if args.mode == "finalize" else {}
    gate = _read(args.gate_report) if args.mode == "finalize" else {}
    entries = [e for e in catalog.get("entries", []) if str(e.get("name")) != name]
    entries.append(_entry(spec, functional, gate)); entries.sort(key=lambda e: str(e.get("name", "")))
    catalog["entries"] = entries; catalog["runtime_api_version"] = "0.17"; catalog["generated_by"] = "acir-runtime-promote-v17"
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old = _read(ROOT / "manifests/parameterized-components-v0.16.json")
    components = [c for c in old.get("components", []) if str(c.get("name")) != name]
    components.append({"name": name, "oracle": spec["oracle"], "parameters": {p["name"]: p.get("source", p.get("derived", "")) for p in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["source_files"], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for e in entries:
        for p in [*e.get("files", []), *e.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(p)
            if path.is_file(): hashes[str(p)] = _sha(path)
    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.17", "release": "runtime-rtl-v0.17", "generated_by": "acir-runtime-promote-v17", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda c: str(c.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = _read(LOCK); sources = lock.setdefault("sources", {}); sources["pulp-common-cells-v0.17"] = {"repository": PULP_REPO, "commit": PULP_COMMIT, "license": "Solderpad-Hardware-License-0.51"}; sources.setdefault("tech-cells-generic-v0.16", {"repository": TECH_REPO, "commit": TECH_COMMIT, "license": "Solderpad-Hardware-License-0.51"}); LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": 1, "accepted": int(entries[-1].get("status") == "accepted")}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
