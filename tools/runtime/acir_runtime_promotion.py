#!/usr/bin/env python3
"""Turn a full RTL validation report into a reviewable runtime manifest.

Validation is intentionally stricter than discovery, but a structural smoke
is not a functional contract.  This module therefore keeps those decisions
separate: every candidate receives a deterministic structural verdict while
the promotion status records whether an interface, provenance and semantic
oracle have also been reviewed.  The output is JSON-only and can be checked
in or consumed by CI without importing MLIR/LLVM.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


SCHEMA = "acir-runtime-promotion-v0.1"
_LICENSES = {
    "basejump_stl": "Solderpad-Hardware-License-0.51",
    "pulp_common_cells": "Solderpad-Hardware-License-0.51",
    "opentitan": "Apache-2.0",
    "vortex": "Apache-2.0",
}


def load_report(path: str | Path) -> dict[str, Any]:
    value = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(value, dict) or not isinstance(value.get("case_results"), list):
        raise ValueError("full validation report must contain a case_results list")
    return value


def _key(candidate: Mapping[str, Any]) -> tuple[str, str, str]:
    return (str(candidate.get("project", "")), str(candidate.get("module", "")), str(candidate.get("source_file", candidate.get("file", ""))))


def _family(project: str, module: str) -> str:
    text = f"{project}/{module}".lower()
    if any(token in text for token in ("arb", "round_robin", "stream_xbar", "crossbar")):
        return "arbitration-interconnect"
    if any(token in text for token in ("fifo", "buffer", "register", "mem_", "memory")):
        return "storage-dataflow"
    if any(token in text for token in ("popcount", "lzc", "leading_zero", "counting_leading")):
        return "reduction"
    if any(token in text for token in ("adder", "sum", "multiplier", "fma", "fp")):
        return "arithmetic"
    if any(token in text for token in ("onehot", "gray", "encoder", "slicer")):
        return "encoding"
    return "other"


def _all(cases: Sequence[Mapping[str, Any]], field: str, values: Iterable[str]) -> bool:
    allowed = set(values)
    return bool(cases) and all(str(case.get(field, "")) in allowed for case in cases)


def _case_summary(cases: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    synthesis = Counter(str(case.get("synthesis", "UNKNOWN")) for case in cases)
    areas = [float(case["mapped_area"]) for case in cases if case.get("mapped_area") is not None]
    return {
        "count": len(cases),
        "configs": [str(case.get("config", "")) for case in cases],
        "synthesis": dict(sorted(synthesis.items())),
        "area_min": min(areas) if areas else None,
        "area_max": max(areas) if areas else None,
        "power_measured": any(case.get("power") is not None for case in cases),
        "semantic_contracts": sorted({str(case.get("semantic_contract", "")) for case in cases}),
    }


def _structural_failure_reasons(cases: Sequence[Mapping[str, Any]], closure: str) -> list[str]:
    """Return deterministic, actionable reasons for a blocked candidate."""
    reasons: set[str] = set()
    if closure != "PASS":
        reasons.add(f"closure:{closure.lower()}")
    for case in cases:
        for field, labels in (
            ("verilator_lint", {"FAIL": "verilator_lint:fail", "TIMEOUT": "verilator_lint:timeout", "BLOCKED_LINT": "verilator_lint:blocked", "NOT_RUN": "verilator_lint:not_run"}),
            ("simulation", {"FAIL": "simulation:fail", "TIMEOUT": "simulation:timeout", "BLOCKED_LINT": "simulation:blocked", "NOT_RUN": "simulation:not_run"}),
            ("synthesis", {"FAIL": "yosys:fail", "TIMEOUT": "yosys:timeout", "BLOCKED_LINT": "yosys:blocked", "NOT_RUN": "yosys:not_run"}),
        ):
            value = str(case.get(field, ""))
            if value in labels:
                reasons.add(labels[value])
    return sorted(reasons)


def build_manifest(report: Mapping[str, Any], *, accept_structural: Sequence[str] = ()) -> dict[str, Any]:
    """Build a deterministic promotion manifest from ``report.json``.

    ``accept_structural`` is an explicit allow-list for projects/modules which
    have a reviewed public adapter but do not yet have a class-specific oracle.
    It is intentionally empty by default; callers can still see all passing
    designs as ``review_required`` without accidentally publishing them.
    """

    candidate_rows = report.get("candidate_results", [])
    case_rows = report.get("case_results", [])
    by_key: dict[str, Mapping[str, Any]] = {str(row.get("case_key")): row for row in case_rows if isinstance(row, Mapping) and row.get("case_key")}
    allow = set(str(item) for item in accept_structural)
    entries: list[dict[str, Any]] = []
    for row in candidate_rows:
        if not isinstance(row, Mapping):
            continue
        cases = [by_key[key] for key in row.get("case_keys", []) if key in by_key]
        project, module, source_file = _key(row)
        if not cases and str(row.get("closure", "")) == "BLOCKED_NON_MODULE":
            cases = [row]
        closure = str(row.get("closure", "UNKNOWN"))
        # A non-synthesizable model can still pass the structural gate when
        # the campaign explicitly records both simulation and synthesis as
        # not applicable.  These are kept separate from synthesizable RTL
        # below so they cannot accidentally enter a synthesis runtime tier.
        structural = (
            closure == "PASS"
            and _all(cases, "verilator_lint", ("PASS",))
            and _all(cases, "simulation", ("PASS", "NOT_APPLICABLE_NON_SYNTH"))
            and _all(cases, "synthesis", ("PASS", "NOT_APPLICABLE_NON_SYNTH"))
        )
        synthesizable = structural and _all(cases, "synthesis", ("PASS",))
        non_synth = structural and _all(cases, "synthesis", ("NOT_APPLICABLE_NON_SYNTH",))
        semantic = bool(cases) and all(str(case.get("semantic_contract", "")) not in {"", "SEMANTIC_CONTRACT_MISSING"} for case in cases)
        interface = bool(cases) and all(not str(case.get("interface", "")).startswith("pyc_synth_top/") for case in cases)
        repo_url = next((str(case.get("repo_url")) for case in cases if case.get("repo_url")), "")
        commit = next((str(case.get("commit_sha")) for case in cases if case.get("commit_sha")), "")
        license_name = _LICENSES.get(project, "")
        provenance = bool(repo_url and commit and source_file and license_name)
        structural_status = "passed" if structural else ("non-module" if closure == "BLOCKED_NON_MODULE" else "failed")
        allow_key = f"{project}/{module}"
        failure_reasons = _structural_failure_reasons(cases, closure)
        if structural and (semantic and interface and provenance):
            promotion = "runtime_ready"
            reason = "all structural, interface, provenance and semantic gates passed"
        elif structural and allow_key in allow and provenance:
            promotion = "accepted_structural"
            reason = "explicit structural allow-list; semantic oracle remains required"
        elif structural:
            promotion = "review_required"
            missing = []
            if not interface:
                missing.append("stable interface adapter")
            if not semantic:
                missing.append("semantic oracle")
            if not provenance:
                missing.append("license/provenance")
            reason = "missing " + ", ".join(missing)
        else:
            promotion = "blocked"
            reason = "one or more required structural gates failed: " + ", ".join(failure_reasons or ["unknown"])
        digest_material = f"{project}|{module}|{source_file}|{commit}".encode("utf-8")
        entries.append({
            "candidate_id": hashlib.sha256(digest_material).hexdigest()[:16],
            "candidate_key": f"{project}|{module}|{source_file}|{commit}",
            "project": project,
            "module": module,
            "source_file": source_file,
            "family": _family(project, module),
            "promotion": promotion,
            "reason": reason,
            "structural_status": structural_status,
            "synthesizable": synthesizable,
            "non_synthesizable": non_synth,
            "interface_reviewed": interface,
            "semantic_status": "verified" if semantic else "missing",
            "license": license_name or None,
            "provenance": {"repository": repo_url or None, "commit": commit or None, "source_file": source_file or None},
            "validation": _case_summary(cases),
            "failure_reasons": failure_reasons,
            "case_keys": list(row.get("case_keys", [])),
        })
    entries.sort(key=lambda item: (item["project"], item["module"], item["source_file"], item["candidate_id"]))
    counts = Counter(item["promotion"] for item in entries)
    return {
        "schema": SCHEMA,
        "generated_from": str(report.get("schema", "acir-runtime-full-validation-v0.1")),
        "generated_at": report.get("generated_at"),
        "accept_structural": sorted(allow),
        "summary": {"candidates": len(entries), "by_promotion": dict(sorted(counts.items())), "structural_pass": sum(item["structural_status"] == "passed" for item in entries)},
        "entries": entries,
    }


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--accept-structural", action="append", default=[], metavar="PROJECT/MODULE")
    args = parser.parse_args(argv)
    try:
        manifest = build_manifest(load_report(args.report), accept_structural=args.accept_structural)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"acir-runtime-promote: error: {exc}", file=sys.stderr)
        return 2
    print(json.dumps({"output": str(args.output.resolve()), **manifest["summary"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
