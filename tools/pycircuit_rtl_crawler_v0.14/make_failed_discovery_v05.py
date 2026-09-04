#!/usr/bin/env python3
"""Export the exact Verilator-failed candidates from a v0.4 aggregate.

The v0.5 rerun intentionally consumes this filtered discovery set instead of
re-running the 206 candidates that already passed the v0.4 lint gate.
"""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


FIELDS = [
    "source_project", "module", "file", "source_priority",
    "discovery_match_score", "target_id", "gap_id",
]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--aggregate", default="build/batch-validation-v0.4/all-batches-report-v0.4.json")
    ap.add_argument("--output-dir", default="build/discovery-v0.5/failed-149-v0.5")
    args = ap.parse_args()
    aggregate = json.loads(Path(args.aggregate).read_text(encoding="utf-8"))
    rows = []
    seen_paths = set()
    for report_path in aggregate.get("reports", []):
        # Aggregate paths were written by a Windows-hosted crawler run; make
        # them consumable from the WSL runner as well.
        raw_path = str(report_path)
        if len(raw_path) >= 3 and raw_path[1:3] == ":\\":
            path = Path("/mnt/" + raw_path[0].lower() + "/" + raw_path[3:].replace("\\", "/"))
        else:
            path = Path(raw_path)
        if str(path) in seen_paths:
            continue
        seen_paths.add(str(path))
        try:
            report = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            continue
        candidate = report.get("candidate", {})
        if report.get("stages", {}).get("verilator_lint", {}).get("status") != "FAIL":
            continue
        rows.append({
            "source_project": candidate.get("source_project", ""),
            "module": candidate.get("module", ""),
            "file": candidate.get("top_file", ""),
            "source_priority": candidate.get("priority", ""),
            "discovery_match_score": candidate.get("discovery_match_score", ""),
            "target_id": candidate.get("target_id", ""),
            "gap_id": candidate.get("gap_id", ""),
        })
    rows.sort(key=lambda r: (r["source_project"], r["module"], r["file"]))
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    with (out / "candidates_failed149.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        w.writeheader()
        w.writerows(rows)
    # Batch inference reads details from the discovery directory sibling.
    detail_source = Path(args.aggregate).parents[1] / "discovery-v0.4" / "all-local-v0.4" / "candidate_details.csv"
    # The path above is relative to build; tolerate invocation from elsewhere.
    if not detail_source.exists():
        detail_source = Path("build/discovery-v0.4/all-local-v0.4/candidate_details.csv")
    details = []
    if detail_source.exists():
        with detail_source.open(newline="", encoding="utf-8") as f:
            by_key = {(r.get("source_project", ""), r.get("module", ""), r.get("file", "")): r for r in csv.DictReader(f)}
        for r in rows:
            d = by_key.get((r["source_project"], r["module"], r["file"]))
            if d:
                details.append(d)
        if details:
            with (out / "candidate_details.csv").open("w", newline="", encoding="utf-8") as f:
                fields = list(details[0].keys())
                w = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
                w.writeheader()
                w.writerows(details)
    print(f"wrote {out / 'candidates_failed149.csv'} ({len(rows)} failed reports)")
    print(f"unique candidate keys: {len({(r['source_project'], r['module'], r['file']) for r in rows})}")


if __name__ == "__main__":
    main()
