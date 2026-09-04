#!/usr/bin/env python3
"""Aggregate resumable v0.4 candidate batches and update target statuses."""
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path


DEFAULT_BATCHES = [
    "priority-000-039-param",
    "priority-040-079",
    "priority-080-119",
    "priority-120-159",
    "priority-160-199",
    "priority-200-239",
    "priority-240-279",
    "priority-280-319",
    "priority-320-354",
]


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def normalize_path(value: str) -> Path:
    # Individual reports produced under WSL contain /mnt/e/... paths, while
    # this aggregator is also usable from native Windows Python.
    if value.startswith("/mnt/") and len(value) > 6:
        drive, rest = value[5], value[7:]
        return Path(f"{drive.upper()}:/{rest}")
    return Path(value)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch-root", default="build/batch-validation-v0.4")
    ap.add_argument("--targets", default="targets.v0.4.materialized.json")
    ap.add_argument("--output", default="build/batch-validation-v0.4/all-batches-report-v0.4.json")
    ap.add_argument("--targets-output", default="targets.v0.4.validated.json")
    ap.add_argument("--batch", action="append", default=[])
    args = ap.parse_args()

    base = Path.cwd()
    batch_root = base / args.batch_root
    batch_names = args.batch or DEFAULT_BATCHES
    reports = []
    missing_batches = []
    for name in batch_names:
        directory = batch_root / name
        if not directory.exists():
            missing_batches.append(name)
            continue
        for path in sorted(directory.glob("*.json")):
            if path.name == "batch_report.json":
                continue
            try:
                report = load(path)
            except Exception:
                continue
            if report.get("schema") != "pycircuit-rtl-candidate-validation-v0.4":
                continue
            report["_report_path"] = str(path)
            reports.append(report)

    # Candidate id is globally stable (project/module/file hash), so duplicate
    # reruns collapse to the newest file deterministically.
    by_id = {r["candidate"]["candidate_id"]: r for r in reports}
    reports = list(by_id.values())
    reports.sort(key=lambda r: (r["candidate"].get("source_project", ""), r["candidate"].get("module", ""), r["candidate"]["candidate_id"]))

    stage_counts = {
        "dependency_closure": Counter(),
        "verilator_lint": Counter(),
        "simulation": Counter(),
        "yosys_synthesis": Counter(),
        "ppa": Counter(),
        "overall": Counter(r.get("overall", "UNKNOWN") for r in reports),
    }
    source_counts = defaultdict(Counter)
    target_candidates = defaultdict(list)
    structural = []
    for report in reports:
        stages = report.get("stages", {})
        for key in stage_counts:
            if key == "overall":
                continue
            stage_counts[key][stages.get(key, {}).get("status", "MISSING")] += 1
        candidate = report["candidate"]
        source = candidate.get("source_project", "")
        source_counts[source][report.get("overall", "UNKNOWN")] += 1
        target_candidates[candidate.get("target_id", "")].append(report)
        if (
            stages.get("verilator_lint", {}).get("status") == "PASS"
            and stages.get("yosys_synthesis", {}).get("status") == "PASS"
        ):
            structural.append(report)

    target_status = {}
    for target_id, candidates in target_candidates.items():
        complete = sum(
            r["stages"].get("dependency_closure", {}).get("status") == "PASS"
            and r["stages"].get("verilator_lint", {}).get("status") == "PASS"
            and r["stages"].get("yosys_synthesis", {}).get("status") == "PASS"
            for r in candidates
        )
        partial = sum(
            r["stages"].get("verilator_lint", {}).get("status") == "PASS"
            and r["stages"].get("yosys_synthesis", {}).get("status") == "PASS"
            for r in candidates
        )
        if complete:
            status = "structural_pass_complete_closure"
        elif partial:
            status = "structural_pass_partial_closure"
        elif any(r["stages"].get("verilator_lint", {}).get("status") == "PASS" for r in candidates):
            status = "verilator_pass_yosys_failed_or_blocked"
        else:
            status = "validation_failed_or_blocked"
        target_status[target_id] = {
            "validation_status": status,
            "candidate_count": len(candidates),
            "structural_pass_count": partial,
            "complete_closure_pass_count": complete,
            "simulation_status": "adapter_required",
        }

    report = {
        "schema": "pycircuit-rtl-all-batches-v0.4",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "batch_root": str(batch_root),
        "batches": batch_names,
        "missing_batches": missing_batches,
        "candidate_count": len(reports),
        "stage_counts": {k: dict(v) for k, v in stage_counts.items()},
        "source_counts": {k: dict(v) for k, v in sorted(source_counts.items())},
        "structural_pass_count": len(structural),
        "functional_simulation": {
            "status": "ADAPTER_REQUIRED",
            "count": len(reports),
            "note": "No generic semantic oracle was inferred for arbitrary RTL interfaces.",
        },
        "target_status": target_status,
        "reports": [r["_report_path"] for r in reports],
    }
    output = base / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    # Update the target manifest while retaining discovery/provenance fields.
    target_path = base / args.targets
    if target_path.exists():
        targets_cfg = load(target_path)
        updated = []
        for target in targets_cfg.get("targets", []):
            target = dict(target)
            status = target_status.get(target.get("target_id"))
            if status:
                target.update(status)
            else:
                target.update(
                    {
                        "validation_status": "no_candidate_validated",
                        "candidate_count": 0,
                        "structural_pass_count": 0,
                        "complete_closure_pass_count": 0,
                        "simulation_status": "adapter_required",
                    }
                )
            updated.append(target)
        targets_cfg.update(
            {
                "schema": "pycircuit-rtl-targets-v0.4-validated",
                "validation_updated_at": datetime.now(timezone.utc).isoformat(),
                "validation_report": str(output),
                "validated_candidate_count": len(reports),
                "validated_structural_pass_count": len(structural),
                "functional_simulation_status": "adapter_required",
                "targets": updated,
            }
        )
        targets_output = base / args.targets_output
        targets_output.write_text(json.dumps(targets_cfg, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(f"wrote {output}")
        print(f"wrote {targets_output}")
    else:
        print(f"wrote {output}; target manifest not found: {target_path}")
    print(json.dumps(report["stage_counts"], ensure_ascii=False))


if __name__ == "__main__":
    main()
