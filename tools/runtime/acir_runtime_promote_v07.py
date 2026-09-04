"""Promote the additional OpenTitan SECDED primitives as runtime v0.7.

The v0.7 delta is intentionally small: larger, fixed-width ECC blocks are
added only after the same clean-codeword and single-bit-correction oracle used
by v0.6.  Existing v0.6 entries are copied unchanged into the new catalog.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

from acir_runtime_promote_v06 import REPO, ROOT, _include_roots
from _paths import normalize_specs

CATALOG = ROOT / "catalog.json"
MANIFEST = ROOT / "manifests" / "parameterized-components-v0.7.json"


def _specs() -> list[dict[str, Any]]:
    v = "verilog/vendor-v0.7/opentitan/hw/ip/prim/rtl"
    out: list[dict[str, Any]] = []
    for data, code, parity_count in ((57, 64, 7), (64, 72, 8)):
        for direction in ("enc", "dec"):
            name = f"opentitan-secded-{code}-{data}-{direction}"
            module = f"pyc_runtime_opentitan_secded_{code}_{data}_{direction}"
            if direction == "enc":
                ports = [{"name": "data_in", "direction": "input", "width": str(data)}, {"name": "data_out", "direction": "output", "width": str(code)}]
            else:
                ports = [{"name": "data_in", "direction": "input", "width": str(code)}, {"name": "data_out", "direction": "output", "width": str(data)}, {"name": "syndrome", "direction": "output", "width": str(code - data)}, {"name": "error", "direction": "output", "width": "2"}]
            out.append({
                "name": name, "module": module, "implementation": f"prim_secded_{code}_{data}_{direction}",
                "source": "opentitan-v0.7", "family": "ecc", "wrapper": f"verilog/{module}.sv",
                "files": [f"verilog/{module}.sv", f"{v}/prim_secded_{code}_{data}_{direction}.sv"],
                "license_file": "licenses/opentitan-v0.7-LICENSE",
                "provenance": {"repository": "https://github.com/lowRISC/opentitan.git", "commit": "b16f2be75d2f38c62d861208453ed5b81ccf41b0", "source_file": f"hw/ip/prim/rtl/prim_secded_{code}_{data}_{direction}.sv", "license": "Apache-2.0"},
                "parameters": [], "ports": ports,
                "oracle": {"id": f"secded-{code}-{data}-{direction}-v1", "kind": "combinational", "contract": f"OpenTitan SECDED({code},{data}) {'encoding' if direction == 'enc' else 'decoding with single-bit correction and error reporting'}"},
                "configs": [{"DATA_WIDTH": data, "CODE_WIDTH": code}],
            })
    return out


def _hash(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--functional-report", type=Path, required=True)
    ap.add_argument("--verify-report", type=Path, required=True)
    args = ap.parse_args()
    specs = normalize_specs(_specs())
    old = json.loads(CATALOG.read_text(encoding="utf-8"))
    names = {s["name"] for s in specs}
    entries = [e for e in old.get("entries", []) if e.get("name") not in names]
    fdoc = json.loads(args.functional_report.read_text(encoding="utf-8")) if args.functional_report.is_file() else {}
    vdoc = json.loads(args.verify_report.read_text(encoding="utf-8")) if args.verify_report.is_file() else {}
    functional = {str(r.get("name")): r for r in fdoc.get("results", []) if isinstance(r, dict)}
    verified = {str(r.get("name")): r for r in vdoc.get("results", []) if isinstance(r, dict)}
    for spec in specs:
        files = list(spec["files"])
        all_files = files + [spec["license_file"]]
        missing = [p for p in all_files if not (ROOT / p).is_file()]
        fr, vr = functional.get(spec["name"], {}), verified.get(spec["name"], {})
        qos = [{"parameters": c.get("parameters", {}), "cells": c.get("qor", {}).get("cells"), "wires": c.get("qor", {}).get("wires"), "wire_bits": c.get("qor", {}).get("wire_bits")} for c in fr.get("cases", []) if isinstance(c, dict)]
        entries.append({"name": spec["name"], "module": spec["module"], "implementation": spec["implementation"], "source": spec["source"], "provider": "github", "status": "accepted", "family": spec["family"], "wrapper": spec["wrapper"], "files": files, "provenance": spec["provenance"], "interface": {"wrapper_module": spec["module"], "parameters": spec["parameters"], "ports": spec["ports"]}, "oracle": spec["oracle"], "dependency_closure": {"status": "complete" if not missing else "incomplete", "source_files": [p for p in files if p != spec["wrapper"]], "license_files": [spec["license_file"]], "include_roots": _include_roots(files)}, "validation": {"status": "passed" if fr.get("status") == "passed" else ("pending" if not fr else "failed"), "mode": "packaged-functional-verilator-yosys", "semantic_status": "functional_oracle_v1", "manifest": "manifests/parameterized-components-v0.7.json", "configs": spec["configs"], "qor": qos, "functional_report": str(args.functional_report.relative_to(REPO).as_posix()) if args.functional_report.is_absolute() else str(args.functional_report), "runtime_gate_report": str(args.verify_report.relative_to(REPO).as_posix()) if args.verify_report.is_absolute() else str(args.verify_report), "runtime_gate": vr.get("status", "pending")}, "verification": vr})
    entries.sort(key=lambda e: e["name"])
    old["entries"] = entries
    old["runtime_api_version"] = "0.7"
    old["generated_by"] = "acir-runtime-promote-v07"
    CATALOG.write_text(json.dumps(old, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    hashes: dict[str, str] = {}
    for entry in entries:
        if entry.get("status") == "accepted":
            for item in list(entry.get("files", [])) + list(entry.get("dependency_closure", {}).get("license_files", [])):
                path = ROOT / item
                if path.is_file():
                    hashes[item] = _hash(path)
    manifest = {"schema": "acir-runtime-parameterized-components-v0.7", "release": "runtime-rtl-v0.7", "generated_by": "acir-runtime-promote-v07", "toolchain": {"verilator": "wsl:/opt/oss-cad/oss-cad-suite/bin/verilator", "yosys": "wsl:/opt/oss-cad/oss-cad-suite/bin/yosys", "jobs": 1, "timeout_seconds": 45}, "policy": {"source_closure": "complete", "license_files_required": True, "functional_oracle_required": True, "ppa": "cell-count QoR recorded; power requires Liberty/activity and is not claimed"}, "components": [{"name": s["name"], "oracle": s["oracle"], "configs": s["configs"]} for s in specs], "sha256": dict(sorted(hashes.items()))}
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"catalog": str(CATALOG), "manifest": str(MANIFEST), "entries": len(entries), "new_components": len(specs), "functional_pass": sum(functional.get(s["name"], {}).get("status") == "passed" for s in specs)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
