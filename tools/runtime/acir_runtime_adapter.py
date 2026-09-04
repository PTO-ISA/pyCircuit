#!/usr/bin/env python3
"""Package structurally validated RTL candidates behind stable adapters.

The full validation campaign deliberately uses the temporary top name
``pyc_synth_top`` so every candidate can be checked in isolation.  This tool
turns those temporary adapters into deterministic, uniquely named candidate
wrappers and manifests.  It does not promote a candidate to the runtime
catalog: semantic oracles, interface review, and source vendoring remain
explicit promotion gates.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
from pathlib import Path
from typing import Any, Mapping, Sequence

try:
    from .acir_runtime_full_validation import run_wsl, stat_metrics, wsl_path
except ImportError:  # pragma: no cover - direct tools/ execution
    from acir_runtime_full_validation import run_wsl, stat_metrics, wsl_path


SCHEMA = "acir-runtime-candidate-adapter-v0.1"
_IDENTIFIER = re.compile(r"[^a-zA-Z0-9_]+")
_VERILOG_IDENTIFIER = re.compile(r"^[a-zA-Z_][a-zA-Z0-9_$]*$")
_PORT = re.compile(
    r"^\s*(input|output|inout)\s+"
    r"(?:(?:logic|wire|reg)\s*)?"
    r"((?:\[[^]]+\]\s*)*)"
    r"([a-zA-Z_]\w*)\s*$"
)


def _read_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"JSON document must be an object: {path}")
    return value


def _write_json(path: Path, value: Mapping[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _slug(value: str) -> str:
    result = _IDENTIFIER.sub("_", value.strip().lower()).strip("_")
    return result or "unknown"


def _candidate_id(project: str, module: str, source_file: str, commit: str) -> str:
    material = f"{project}|{module}|{source_file}|{commit}".encode("utf-8")
    return hashlib.sha256(material).hexdigest()[:16]


def _wrapper_name(entry: Mapping[str, Any]) -> str:
    project = _slug(str(entry.get("project", "candidate")))
    module = _slug(str(entry.get("module", "module")))
    candidate_id = str(entry.get("candidate_id", ""))[:8]
    suffix = f"_{candidate_id}" if candidate_id else ""
    return f"pyc_candidate_{project}_{module}{suffix}"


def _case_is_structural_pass(case: Mapping[str, Any]) -> bool:
    return (
        str(case.get("closure", "")) == "PASS"
        and str(case.get("verilator_lint", "")) == "PASS"
        and str(case.get("simulation", "")) in {"PASS", "NOT_APPLICABLE_NON_SYNTH"}
        and str(case.get("synthesis", "")) in {"PASS", "NOT_APPLICABLE_NON_SYNTH"}
    )


def _extract_ports(wrapper: str, module_name: str) -> list[dict[str, str]]:
    match = re.search(
        rf"\bmodule\s+{re.escape(module_name)}\s*\((.*?)\)\s*;",
        wrapper,
        flags=re.DOTALL,
    )
    if not match:
        return []
    ports: list[dict[str, str]] = []
    for raw_line in match.group(1).splitlines():
        line = raw_line.split("//", 1)[0].strip().rstrip(",")
        parsed = _PORT.match(line)
        if not parsed:
            continue
        direction, width, name = parsed.groups()
        ports.append({
            "name": name,
            "direction": direction,
            "width": " ".join(width.split()) or "1",
        })
    return ports


def _check_verilog_interface(wrapper: str, module_name: str) -> dict[str, Any]:
    """Apply the conservative public-module checks used for promotion.

    A structural candidate may contain arbitrary SystemVerilog internally, but
    its adapter boundary must be a legal module with scalar/packed ports.  A
    wrapper with no public ports is retained as an explicit closed smoke
    adapter rather than treated as a flat runtime API.
    """
    issues: list[str] = []
    if not _VERILOG_IDENTIFIER.fullmatch(module_name):
        issues.append("wrapper module name is not a Verilog identifier")
    declarations = re.findall(rf"\bmodule\s+{re.escape(module_name)}\b", wrapper)
    if len(declarations) != 1:
        issues.append(f"expected exactly one wrapper module declaration, found {len(declarations)}")
    if not re.search(rf"\bendmodule\b", wrapper):
        issues.append("wrapper has no endmodule")
    match = re.search(rf"\bmodule\s+{re.escape(module_name)}\s*\((.*?)\)\s*;", wrapper, flags=re.DOTALL)
    ports = _extract_ports(wrapper, module_name)
    if match and match.group(1).strip() and not ports:
        issues.append("port list is not composed of standard input/output/inout declarations")
    if match:
        for raw_line in match.group(1).splitlines():
            line = raw_line.split("//", 1)[0].strip().rstrip(",")
            if not line:
                continue
            parsed = _PORT.match(line)
            if not parsed:
                issues.append(f"unsupported port declaration: {line}")
                continue
            if "interface" in line.lower():
                issues.append(f"interface or unpacked aggregate port: {parsed.group(3)}")
    if issues:
        status = "rejected"
    elif ports:
        status = "standard_verilog_flat"
    else:
        status = "closed_interface_smoke"
    return {"status": status, "issues": issues, "ports": ports}


def _case_dirs(validation_root: Path, entry: Mapping[str, Any], cases: Mapping[str, Mapping[str, Any]]) -> list[tuple[Path, Mapping[str, Any]]]:
    selected: list[tuple[Path, Mapping[str, Any]]] = []
    # A candidate is only structurally promotable when every requested sweep
    # point passes.  Do not silently package a module with one passing config
    # while another config failed lint/simulation/synthesis.
    requested = list(entry.get("case_keys", []))
    for key in entry.get("case_keys", []):
        case = cases.get(str(key))
        if not case or not _case_is_structural_pass(case):
            return []
        case_dir = (
            validation_root
            / "cases"
            / str(case.get("project", entry.get("project", "")))
            / str(case.get("module", entry.get("module", "")))
            / str(case.get("config", ""))
        )
        wrapper = case_dir / "adapter_top.sv"
        if wrapper.is_file():
            selected.append((case_dir, case))
    return selected if len(selected) == len(requested) else []


def _manifest_entry(
    entry: Mapping[str, Any],
    wrapper_name: str,
    wrapper_path: Path,
    selected: Sequence[tuple[Path, Mapping[str, Any]]],
    candidate_manifest: Mapping[str, Any] | None,
) -> dict[str, Any]:
    first_case = selected[0][1]
    ports = _extract_ports((selected[0][0] / "adapter_top.sv").read_text(encoding="utf-8"), "pyc_synth_top")
    provenance = {
        "repository": first_case.get("repo_url"),
        "commit": first_case.get("commit_sha"),
        "source_file": first_case.get("source_file"),
        "license": entry.get("license"),
    }
    interface_kind = "structural_flattened" if ports else "closed_interface_smoke"
    dependency: dict[str, Any] = {
        "candidate_filelist": None,
        "closure_status": first_case.get("closure"),
        "source_files": [],
        "include_roots": [],
    }
    if candidate_manifest:
        candidate_filelist = candidate_manifest.get("candidate_filelist")
        if candidate_filelist:
            dependency["candidate_filelist"] = str(candidate_filelist)
        dependency["source_files"] = [
            *[str(item) for item in candidate_manifest.get("module_files", [])],
            *[str(item) for item in candidate_manifest.get("header_files", [])],
            *[str(item) for item in candidate_manifest.get("package_files", [])],
        ]
        dependency["include_roots"] = [str(item) for item in candidate_manifest.get("include_roots", [])]
    return {
        "schema": SCHEMA,
        "name": f"candidate/{entry.get('project', 'unknown')}/{entry.get('module', 'unknown')}",
        "status": "staged_structural",
        "promotion": entry.get("promotion", "review_required"),
        "family": entry.get("family"),
        "implementation": entry.get("module"),
        "wrapper": wrapper_path.as_posix(),
        "wrapper_module": wrapper_name,
        "interface": {
            "wrapper_module": wrapper_name,
            "kind": interface_kind,
            "parameters": first_case.get("parameters", {}),
            "ports": ports,
        },
        "sweep_configs": [
            {
                "config": case.get("config"),
                "parameters": case.get("parameters", {}),
                "verilator_lint": case.get("verilator_lint"),
                "simulation": case.get("simulation"),
                "synthesis": case.get("synthesis"),
                "mapped_area": case.get("mapped_area"),
                "logic_depth": case.get("logic_depth"),
            }
            for _, case in selected
        ],
        "provenance": provenance,
        "dependency": dependency,
        "validation": {
            "source": "acir-runtime-full-validation-v0.1",
            "semantic_status": entry.get("semantic_status", "missing"),
            "interface_reviewed": bool(entry.get("interface_reviewed", False)),
            "structural_status": entry.get("structural_status"),
            "case_count": len(selected),
            "note": "Structural adapter only; no semantic contract is claimed.",
        },
    }


def build_adapters(report: Mapping[str, Any], promotion: Mapping[str, Any], validation_root: Path, output: Path) -> dict[str, Any]:
    cases = {
        str(row.get("case_key")): row
        for row in report.get("case_results", [])
        if isinstance(row, Mapping) and row.get("case_key")
    }
    candidate_rows = {
        str(row.get("candidate_id")): row
        for row in promotion.get("entries", [])
        if isinstance(row, Mapping) and row.get("candidate_id")
    }
    wrappers_dir = output / "wrappers"
    manifests_dir = output / "manifests"
    wrappers_dir.mkdir(parents=True, exist_ok=True)
    manifests_dir.mkdir(parents=True, exist_ok=True)
    entries: list[dict[str, Any]] = []
    counters = {
        "staged_structural": 0,
        "blocked_missing_wrapper": 0,
        "skipped_non_structural": 0,
        "standard_verilog_flat": 0,
        "closed_interface_smoke": 0,
        "interface_rejected": 0,
    }
    for candidate in sorted(candidate_rows.values(), key=lambda item: str(item.get("candidate_id"))):
        candidate_id = str(candidate.get("candidate_id"))
        selected = _case_dirs(validation_root, candidate, cases)
        if not selected:
            counters["blocked_missing_wrapper"] += 1
            continue
        wrapper_name = _wrapper_name(candidate)
        wrapper_text = (selected[0][0] / "adapter_top.sv").read_text(encoding="utf-8")
        renamed, replacements = re.subn(r"\bmodule\s+pyc_synth_top\b", f"module {wrapper_name}", wrapper_text, count=1)
        if replacements != 1:
            counters["blocked_missing_wrapper"] += 1
            continue
        header = (
            "// Generated by acir-runtime-adapt.py from a validated structural adapter.\n"
            f"// Candidate: {candidate.get('project')}/{candidate.get('module')} ({candidate_id})\n"
            "// This wrapper preserves the flattened ports of one validation point; "
            "it is not a semantic API until reviewed.\n"
        )
        wrapper_path = wrappers_dir / f"{wrapper_name}.sv"
        wrapper_path.write_text(header + renamed, encoding="utf-8")
        interface_check = _check_verilog_interface(renamed, wrapper_name)
        candidate_manifest_path = (
            validation_root
            / "candidate-builds"
            / str(candidate.get("project", ""))
            / str(candidate.get("module", ""))
            / "manifest.json"
        )
        candidate_manifest = _read_json(candidate_manifest_path) if candidate_manifest_path.is_file() else None
        manifest = _manifest_entry(candidate, wrapper_name, wrapper_path.relative_to(output), selected, candidate_manifest)
        manifest["interface_check"] = interface_check
        _write_json(manifests_dir / f"{wrapper_name}.json", manifest)
        entries.append(manifest)
        counters["staged_structural"] += 1
        counters[interface_check["status"]] = counters.get(interface_check["status"], 0) + 1
        if interface_check["status"] == "rejected":
            counters["interface_rejected"] += 1
    result = {
        "schema": SCHEMA,
        "generated_from": {
            "validation_report": str((validation_root / "report.json").resolve()),
            "promotion_manifest": str((validation_root / "runtime-candidates.json").resolve()),
        },
        "summary": {
            "candidates_seen": len(candidate_rows),
            **counters,
            "entries": len(entries),
        },
        "entries": entries,
    }
    _write_json(output / "catalog.json", result)
    return result


def verify_adapters(catalog: dict[str, Any], validation_root: Path, output: Path, timeout: int) -> dict[str, Any]:
    """Re-run lint and synthesis using each uniquely named staged wrapper.

    The expensive parameter sweep and generic simulation are already recorded
    by the full validation campaign.  This pass checks that renaming the
    temporary ``pyc_synth_top`` does not change elaboration and synthesis, and
    stores fresh logs under the adapter output directory.  Non-synthesizable
    candidates retain their explicit NOT_APPLICABLE result.
    """
    verification_root = output / "verification"
    rows: list[dict[str, Any]] = []
    for entry in catalog.get("entries", []):
        if not isinstance(entry, dict):
            continue
        project = str(entry.get("provenance", {}).get("project", entry.get("project", "")))
        # The project/module fields are represented in the stable candidate
        # name and provenance in v0.1; recover them from the catalog name.
        name_parts = str(entry.get("name", "")).split("/")
        if len(name_parts) >= 3 and name_parts[0] == "candidate":
            project, module = name_parts[1], name_parts[2]
        else:
            project = str(entry.get("project", project))
            module = str(entry.get("implementation", ""))
        wrapper_name = str(entry.get("wrapper_module", ""))
        wrapper_path = output / str(entry.get("wrapper", ""))
        sweeps = entry.get("sweep_configs", [])
        config = str(sweeps[0].get("config", "s0")) if sweeps and isinstance(sweeps[0], dict) else "s0"
        source_case = validation_root / "cases" / project / module / config
        candidate_dir = validation_root / "candidate-builds" / project / module
        candidate_filelist = candidate_dir / "candidate.f"
        verification_dir = verification_root / wrapper_name
        verification_dir.mkdir(parents=True, exist_ok=True)
        row: dict[str, Any] = {
            "name": entry.get("name"),
            "wrapper_module": wrapper_name,
            "project": project,
            "module": module,
            "config": config,
            "verilator_lint": "NOT_RUN",
            "synthesis": "NOT_RUN",
            "cells": None,
            "logic_depth": None,
            "mapped_area": None,
        }
        if not wrapper_path.is_file() or not candidate_filelist.is_file():
            row["verilator_lint"] = "BLOCKED_INPUT"
            row["synthesis"] = "BLOCKED_INPUT"
            rows.append(row)
            continue
        defines = "-DSYNTHESIS -DVX_CFG_XLEN=32" if project == "vortex" else ""
        lint_cmd = (
            "verilator --lint-only --Wno-fatal --top-module "
            + shlex.quote(wrapper_name)
            + " "
            + defines
            + " -F "
            + shlex.quote(wsl_path(candidate_filelist))
            + " "
            + shlex.quote(wsl_path(wrapper_path))
        )
        lint = run_wsl(lint_cmd, timeout)
        (verification_dir / "verilator_lint_stdout.log").write_text(lint.get("stdout", ""), encoding="utf-8")
        (verification_dir / "verilator_lint_stderr.log").write_text(lint.get("stderr", ""), encoding="utf-8")
        row["verilator_lint"] = lint.get("status", "ERROR")
        row["verilator_lint_elapsed_s"] = lint.get("elapsed_s")
        if row["verilator_lint"] != "PASS":
            rows.append(row)
            continue
        prior_case = source_case / "generic.ys"
        original_wrapper = source_case / "adapter_top.sv"
        all_non_synth = bool(sweeps) and all(
            isinstance(item, dict) and item.get("synthesis") == "NOT_APPLICABLE_NON_SYNTH"
            for item in sweeps
        )
        if all_non_synth:
            row["synthesis"] = "NOT_APPLICABLE_NON_SYNTH"
            row["note"] = "source campaign marked this candidate non-synthesizable"
            rows.append(row)
            continue
        if not prior_case.is_file():
            row["synthesis"] = "BLOCKED_INPUT"
            rows.append(row)
            continue
        script_text = prior_case.read_text(encoding="utf-8")
        # Resumed campaigns can retain a generic.ys generated in an earlier
        # output directory.  Recover its case directory from the embedded
        # adapter path instead of assuming it equals ``validation_root``.
        old_adapter = re.search(r"/mnt/[^ \"\n]+/adapter_top\.sv", script_text)
        if old_adapter:
            old_case = old_adapter.group(0).rsplit("/", 1)[0]
            script_text = script_text.replace(old_adapter.group(0), wsl_path(wrapper_path))
            script_text = script_text.replace(old_case, wsl_path(verification_dir))
        else:
            script_text = script_text.replace(wsl_path(original_wrapper), wsl_path(wrapper_path))
            script_text = script_text.replace(wsl_path(source_case), wsl_path(verification_dir))
        script_text = re.sub(r"\bpyc_synth_top\b", wrapper_name, script_text)
        script_path = verification_dir / "generic.ys"
        script_path.write_text(script_text, encoding="utf-8")
        synth = run_wsl("yosys -s " + shlex.quote(wsl_path(script_path)), timeout)
        (verification_dir / "yosys_stdout.log").write_text(synth.get("stdout", ""), encoding="utf-8")
        (verification_dir / "yosys_stderr.log").write_text(synth.get("stderr", ""), encoding="utf-8")
        row["synthesis"] = synth.get("status", "ERROR")
        row["synthesis_elapsed_s"] = synth.get("elapsed_s")
        if row["synthesis"] == "PASS":
            cells, depth, area = stat_metrics(verification_dir / "stats.json")
            _, _, mapped_area = stat_metrics(verification_dir / "mapped_stats.json")
            row.update({"cells": cells, "logic_depth": depth, "mapped_area": mapped_area if mapped_area is not None else area})
        rows.append(row)
    summary = {
        "entries": len(rows),
        "verilator_lint_pass": sum(row.get("verilator_lint") == "PASS" for row in rows),
        "synthesis_pass": sum(row.get("synthesis") == "PASS" for row in rows),
        "synthesis_not_applicable": sum(row.get("synthesis") == "NOT_APPLICABLE_NON_SYNTH" for row in rows),
        "failed_or_blocked": sum(row.get("verilator_lint") not in {"PASS"} or row.get("synthesis") not in {"PASS", "NOT_APPLICABLE_NON_SYNTH"} for row in rows),
    }
    verification = {"schema": "acir-runtime-candidate-adapter-verification-v0.1", "summary": summary, "results": rows}
    _write_json(output / "verification.json", verification)
    for entry in catalog.get("entries", []):
        if not isinstance(entry, dict):
            continue
        match = next((row for row in rows if row.get("wrapper_module") == entry.get("wrapper_module")), None)
        if match:
            entry["verification"] = match
    _write_json(output / "catalog.json", catalog)
    return verification


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", required=True, type=Path, help="full validation report.json")
    parser.add_argument("--promotion", required=True, type=Path, help="runtime-candidates.json")
    parser.add_argument("--validation-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--verify", action="store_true", help="re-run Verilator lint and Yosys for staged wrappers")
    parser.add_argument("--tool-timeout", type=int, default=45)
    args = parser.parse_args(argv)
    report = _read_json(args.report)
    promotion = _read_json(args.promotion)
    result = build_adapters(report, promotion, args.validation_root.resolve(), args.output.resolve())
    if args.verify:
        verification = verify_adapters(result, args.validation_root.resolve(), args.output.resolve(), max(1, args.tool_timeout))
        result["verification"] = verification["summary"]
    print(json.dumps({"output": str(args.output.resolve()), **result["summary"], **({"verification": result["verification"]} if "verification" in result else {})}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
