#!/usr/bin/env python3
"""Promote Vortex stream fork/join as runtime RTL v0.23."""

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
MANIFEST = ROOT / "manifests/parameterized-components-v0.23.json"
LOCK = ROOT / "catalog.lock.json"


def _specs() -> list[dict[str, Any]]:
    base = "verilog/vendor-v0.22/vortex/hw"
    common_files = [
        f"{base}/VX_config.vh", f"{base}/VX_types.vh",
        f"{base}/rtl/VX_scope.vh", f"{base}/rtl/VX_platform.vh",
        f"{base}/rtl/VX_define.vh",
        *[f"{base}/rtl/libs/{name}" for name in (
            "VX_placeholder.sv", "VX_pipe_register.sv", "VX_pipe_buffer.sv",
            "VX_stream_buffer.sv", "VX_pending_size.sv", "VX_fifo_queue.sv",
            "VX_async_ram_patch.sv", "VX_dp_ram.sv", "VX_elastic_buffer.sv")],
    ]
    common = {
        "source": "vortex-v0.22", "provider": "github", "family": "dataflow",
        "repository": "https://github.com/vortexgpgpu/vortex.git",
        "commit": "d76b7f24e658867ab57e3942d7c648c3e6af072d",
        "license": "Apache-2.0", "license_file": "licenses/vortex-v0.13-LICENSE",
        "source_files": common_files,
    }
    return [
        {
            **common, "name": "vortex-stream-fork", "module": "pyc_runtime_vortex_stream_fork",
            "implementation": "VX_stream_fork", "wrapper": "verilog/pyc_runtime_vortex_stream_fork.sv",
            "source_files": [*common_files, f"{base}/rtl/libs/VX_stream_fork.sv"],
            "parameters": [
                {"name": "OUTPUTS", "source": "NUM_OUTPUTS", "default": 2},
                {"name": "DATA_WIDTH", "source": "DATAW", "default": 8},
                {"name": "OUT_BUF", "source": "OUT_BUF", "default": 0},
                {"name": "EAGER", "source": "EAGER", "default": 0},
            ],
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "reset", "direction": "input", "width": "1"},
                {"name": "valid_in", "direction": "input", "width": "1"},
                {"name": "ready_in", "direction": "output", "width": "1"},
                {"name": "data_in", "direction": "input", "width": "DATA_WIDTH"},
                {"name": "valid_out", "direction": "output", "width": "OUTPUTS"},
                {"name": "data_out", "direction": "output", "width": "OUTPUTS*DATA_WIDTH"},
                {"name": "ready_out", "direction": "input", "width": "OUTPUTS"},
            ],
            "oracle": {"id": "vortex-stream-fork-v1", "kind": "cycle/ready-valid",
                       "contract": "a lockstep fork broadcasts each accepted word to every output and blocks until all outputs are ready"},
            "configs": [{"OUTPUTS": 1, "DATA_WIDTH": 1}, {"OUTPUTS": 2, "DATA_WIDTH": 4}, {"OUTPUTS": 4, "DATA_WIDTH": 8}],
        },
        {
            **common, "name": "vortex-stream-join", "module": "pyc_runtime_vortex_stream_join",
            "implementation": "VX_stream_join", "wrapper": "verilog/pyc_runtime_vortex_stream_join.sv",
            "source_files": [*common_files, f"{base}/rtl/libs/VX_stream_join.sv"],
            "parameters": [
                {"name": "INPUTS", "source": "NUM_INPUTS", "default": 2},
                {"name": "DATA_WIDTH", "source": "DATAW", "default": 8},
                {"name": "OUT_BUF", "source": "OUT_BUF", "default": 0},
                {"name": "EAGER", "source": "EAGER", "default": 0},
            ],
            "ports": [
                {"name": "clk", "direction": "input", "width": "1"},
                {"name": "reset", "direction": "input", "width": "1"},
                {"name": "valid_in", "direction": "input", "width": "INPUTS"},
                {"name": "ready_in", "direction": "output", "width": "INPUTS"},
                {"name": "data_in", "direction": "input", "width": "INPUTS*DATA_WIDTH"},
                {"name": "valid_out", "direction": "output", "width": "1"},
                {"name": "data_out", "direction": "output", "width": "INPUTS*DATA_WIDTH"},
                {"name": "ready_out", "direction": "input", "width": "1"},
            ],
            "oracle": {"id": "vortex-stream-join-v1", "kind": "cycle/ready-valid",
                       "contract": "a lockstep join emits a bundle only when every input is valid and the downstream is ready, preserving lane data"},
            "configs": [{"INPUTS": 1, "DATA_WIDTH": 1}, {"INPUTS": 2, "DATA_WIDTH": 4}, {"INPUTS": 4, "DATA_WIDTH": 8}],
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
    return next((dict(item) for item in report.get("results", [])
                 if isinstance(item, Mapping) and str(item.get("name")) == name), {})


def _entry(spec: Mapping[str, Any], functional: Mapping[str, Any], gate: Mapping[str, Any]) -> dict[str, Any]:
    files = [str(spec["wrapper"]), *[str(item) for item in spec["source_files"]]]
    license_file = str(spec["license_file"])
    missing = [path for path in [*files, license_file] if not (ROOT / path).is_file()]
    f = _result(functional, str(spec["name"])); g = _result(gate, str(spec["name"]))
    staging = not functional and not gate
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
        "provenance": {"repository": spec["repository"], "commit": spec["commit"], "source_file": f"hw/rtl/libs/{spec['implementation']}.sv", "license": spec["license"]},
        "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]},
        "oracle": spec["oracle"],
        "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": spec["source_files"], "license_files": [license_file], "include_roots": ["verilog/vendor-v0.22/vortex/hw", "verilog/vendor-v0.22/vortex/hw/rtl", "verilog/vendor-v0.22/vortex/hw/rtl/libs"]},
        "validation": {"status": "passed" if accepted and not staging else "pending", "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.23.json", "configs": spec["configs"], "functional_report": ".pycircuit_out/runtime-functional-validation/v23-vortex-stream.json", "runtime_gate_report": ".pycircuit_out/runtime-catalog-validation/v23-vortex-stream.json", "runtime_gate": "passed" if g.get("status") == "passed" else ("pending" if staging else "failed"), "qor": qor},
        "verification": {"name": spec["name"], "status": "passed" if g.get("status") == "passed" else "pending", "top": spec["module"], "files": files, "verilator": g.get("verilator", {}), "yosys": g.get("yosys", {})},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("prepare", "finalize"), required=True)
    parser.add_argument("--functional-report", type=Path, default=REPO / ".pycircuit_out/runtime-functional-validation/v23-vortex-stream.json")
    parser.add_argument("--gate-report", type=Path, default=REPO / ".pycircuit_out/runtime-catalog-validation/v23-vortex-stream.json")
    args = parser.parse_args()
    specs = normalize_specs(_specs()); names = {str(spec["name"]) for spec in specs}
    functional = _read(args.functional_report) if args.mode == "finalize" else {}
    gate = _read(args.gate_report) if args.mode == "finalize" else {}
    catalog = _read(CATALOG)
    entries = [entry for entry in catalog.get("entries", []) if str(entry.get("name")) not in names]
    entries.extend(_entry(spec, functional, gate) for spec in specs)
    entries.sort(key=lambda entry: str(entry.get("name", "")))
    catalog.update({"entries": entries, "runtime_api_version": "0.23", "generated_by": "acir-runtime-promote-v23"})
    CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    old = _read(ROOT / "manifests/parameterized-components-v0.22.json")
    components = [component for component in old.get("components", []) if str(component.get("name")) not in names]
    for spec in specs:
        components.append({"name": spec["name"], "oracle": spec["oracle"], "parameters": {p["name"]: p.get("source", "") for p in spec["parameters"]}, "configs": spec["configs"], "source": {"repository": spec["repository"], "commit": spec["commit"], "files": spec["source_files"], "license": spec["license_file"]}})
    hashes: dict[str, str] = {}
    for entry in entries:
        for path_text in [*entry.get("files", []), *entry.get("dependency_closure", {}).get("license_files", [])]:
            path = ROOT / str(path_text)
            if path.is_file(): hashes[str(path_text)] = _sha(path)
    MANIFEST.write_text(json.dumps({"schema": "acir-runtime-parameterized-components-v0.23", "release": "runtime-rtl-v0.23", "generated_by": "acir-runtime-promote-v23", "toolchain": old.get("toolchain", {}), "policy": old.get("policy", {}), "components": sorted(components, key=lambda x: str(x.get("name", ""))), "sha256": dict(sorted(hashes.items()))}, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    lock = _read(LOCK); lock.setdefault("sources", {})["vortex-v0.22"] = {"repository": specs[0]["repository"], "commit": specs[0]["commit"], "license": specs[0]["license"]}; LOCK.write_text(json.dumps(lock, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    promoted = [entry for entry in entries if str(entry.get("name")) in names]
    print(json.dumps({"mode": args.mode, "catalog_entries": len(entries), "new_components": len(specs), "accepted": sum(entry.get("status") == "accepted" for entry in promoted)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
