#!/usr/bin/env python3
"""Run Verilator and Yosys gates for every accepted runtime catalog entry.

The crawler validates candidate closures while they are discovered.  This
small, serial checker validates the *packaged* runtime surface that is shipped
with Agentic Circuit.  It deliberately resolves every path before invoking a
WSL tool, so Windows relative paths cannot be mangled by the WSL boundary.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Mapping, Sequence

try:  # Package import when embedded by another tool.
    from .acir_runtime_crawler import validate_candidate
except ImportError:  # Direct ``python tools/acir_runtime_verify.py`` usage.
    from acir_runtime_crawler import validate_candidate


def _load_catalog(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict) or not isinstance(value.get("entries"), list):
        raise ValueError("catalog must be an object with an entries array")
    return value


def _entry_files(entry: Mapping[str, Any], catalog_path: Path) -> tuple[str, list[Path]]:
    interface = entry.get("interface")
    interface = interface if isinstance(interface, Mapping) else {}
    wrapper = str(entry.get("wrapper", ""))
    files = entry.get("files")
    raw_files = [str(item) for item in files] if isinstance(files, list) else []
    if not raw_files and wrapper:
        raw_files = [wrapper]
    if not raw_files:
        module = str(entry.get("module", ""))
        raw_files = [f"verilog/{module}.v"] if module else []
    if wrapper and wrapper not in raw_files:
        raw_files.insert(0, wrapper)
    root = catalog_path.parent
    resolved: list[Path] = []
    for item in raw_files:
        path = Path(item)
        if not path.is_absolute():
            path = root / path
        resolved.append(path.resolve())
    # SystemVerilog packages must be declared before a module that imports
    # them (notably the vendored common_cells cc_lzc).  Catalog entries keep
    # their human-readable file list, while the execution order is normalized
    # here for all consumers of the runtime verifier.
    resolved = sorted(
        dict.fromkeys(resolved),
        key=lambda path: (0 if path.name in {"cc_pkg.sv"} else 1, str(path)),
    )
    top = str(interface.get("wrapper_module") or entry.get("module") or "")
    return top, resolved


def _entry_include_dirs(files: Sequence[Path]) -> list[Path]:
    """Derive include roots without treating header files as HDL units.

    A vendored path such as ``pulp/include/common_cells/assertions.svh`` must
    be searched from ``pulp/include`` for `` `include "common_cells/..." ``.
    Passing the leaf ``common_cells`` directory instead produces a duplicated
    path and makes Verilator report missing assertion macros.
    """
    roots: list[Path] = []
    for path in files:
        parts = list(path.resolve().parts)
        for index, part in enumerate(parts):
            if part.lower() == "include":
                roots.append(Path(*parts[: index + 1]))
        roots.append(path.parent.resolve())
    return list(dict.fromkeys(roots))


def _compact_gate(gate: Mapping[str, Any]) -> dict[str, Any]:
    result: dict[str, Any] = {key: gate[key] for key in ("status", "returncode", "elapsed_s", "qor", "reason") if key in gate}
    if gate.get("status") not in {"passed", "skipped"}:
        for key in ("stderr", "stdout", "error"):
            if gate.get(key):
                result[key] = str(gate[key])[-2000:]
    return result


def verify_catalog(
    catalog_path: Path,
    *,
    verilator: str | None,
    yosys: str | None,
    timeout: int,
    selected: Sequence[str] = (),
    no_tools: bool = False,
) -> dict[str, Any]:
    catalog = _load_catalog(catalog_path)
    wanted = set(selected)
    results: list[dict[str, Any]] = []
    for entry in catalog["entries"]:
        if not isinstance(entry, Mapping) or entry.get("status") != "accepted":
            continue
        name = str(entry.get("name", ""))
        if wanted and name not in wanted:
            continue
        top, files = _entry_files(entry, catalog_path)
        missing = [str(path) for path in files if not path.is_file()]
        if not top:
            missing.append("missing wrapper_module/module")
        if missing:
            results.append({"name": name, "status": "missing-files", "top": top, "files": [str(path) for path in files], "missing": missing})
            continue
        if no_tools:
            results.append({"name": name, "status": "skipped", "top": top, "files": [str(path) for path in files], "reason": "tools disabled"})
            continue
        gates = validate_candidate(
            files,
            top,
            verilator=verilator,
            yosys=yosys,
            timeout=timeout,
            include_dirs=_entry_include_dirs(files),
        )
        results.append({
            "name": name,
            "status": "passed" if gates.get("status") == "passed" else ("skipped" if gates.get("status") == "skipped" else "failed"),
            "top": top,
            "files": [str(path) for path in files],
            "verilator": _compact_gate(gates.get("verilator", {})),
            "yosys": _compact_gate(gates.get("yosys", {})),
        })
    counts: dict[str, int] = {}
    for result in results:
        counts[result["status"]] = counts.get(result["status"], 0) + 1
    overall = "passed" if counts.get("failed", 0) == 0 and counts.get("missing-files", 0) == 0 else "failed"
    if results and counts.get("passed", 0) == 0 and counts.get("skipped", 0) == len(results):
        overall = "skipped"
    return {"schema": "acir-runtime-validation-v0.1", "catalog": str(catalog_path), "summary": {"entries": len(results), "status": overall, "counts": counts}, "results": results}


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--catalog", type=Path, default=Path("library/verilog/catalog.json"))
    parser.add_argument("--report", type=Path, default=Path(".pycircuit_out/runtime-catalog-validation/report.json"))
    parser.add_argument("--verilator", default="verilator", help="Verilator executable or wsl:verilator")
    parser.add_argument("--yosys", default="yosys", help="Yosys executable or wsl:yosys")
    parser.add_argument("--timeout", type=int, default=45, help="timeout per tool gate in seconds")
    parser.add_argument("--entry", action="append", default=[], help="validate only this catalog name; repeatable")
    parser.add_argument("--no-tools", action="store_true", help="only check packaged files and interface metadata")
    args = parser.parse_args(argv)
    catalog_path = args.catalog.resolve()
    try:
        report = verify_catalog(catalog_path, verilator=None if args.no_tools else args.verilator, yosys=None if args.no_tools else args.yosys, timeout=max(1, args.timeout), selected=args.entry, no_tools=args.no_tools)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"acir-runtime verify-runtime: error: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(report["summary"], sort_keys=True))
    return 0 if report["summary"]["status"] in {"passed", "skipped"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
