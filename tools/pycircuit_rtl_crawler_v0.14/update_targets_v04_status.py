#!/usr/bin/env python3
"""Attach local source-pool and discovery status to a v0.4 target manifest."""
from __future__ import annotations

import argparse
import csv
import json
from datetime import datetime, timezone
from pathlib import Path


def rows(path: Path):
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--targets", default="targets.v0.4.json")
    ap.add_argument("--sources", default="build/source-cache-v0.4/source-manifest-v0.4.json")
    ap.add_argument("--discovery", default="build/discovery-v0.4/all-local-v0.4/candidates_raw.csv")
    ap.add_argument("--output", default="targets.v0.4.materialized.json")
    args = ap.parse_args()

    target_cfg = json.loads(Path(args.targets).read_text(encoding="utf-8"))
    source_cfg = json.loads(Path(args.sources).read_text(encoding="utf-8"))
    hits = rows(Path(args.discovery))
    by_target = {}
    modules_by_target = {}
    for hit in hits:
        tid = hit.get("target_id", "")
        by_target[tid] = by_target.get(tid, 0) + 1
        modules_by_target.setdefault(tid, set()).add(
            (hit.get("source_project", ""), hit.get("module", ""), hit.get("file", ""))
        )

    source_status = {s["project"]: s for s in source_cfg.get("sources", [])}
    out_targets = []
    for original in target_cfg.get("targets", []):
        target = dict(original)
        tid = target.get("target_id", "")
        count = by_target.get(tid, 0)
        target["candidate_count"] = count
        target["candidate_module_count"] = len(modules_by_target.get(tid, set()))
        target["scan_status"] = "matched" if count else "no_match"
        target["runtime_status"] = "discovered_candidate" if count else "search_candidate_only"
        # The source pool is local even when a source has no direct RTL (e.g.
        # Chisel/Scala-only repositories); validation remains a later stage.
        target["clone_status"] = "materialized"
        target["validation_status"] = "not_run"
        target["candidate_source_status"] = {
            p: source_status.get(p, {}).get("content_status", "unknown")
            for p in target.get("candidate_source_projects", [])
        }
        out_targets.append(target)

    result = dict(target_cfg)
    result.update(
        {
            "schema": "pycircuit-rtl-targets-v0.4-materialized",
            "status_updated_at": datetime.now(timezone.utc).isoformat(),
            "source_manifest": str(Path(args.sources)),
            "discovery_report": str(Path(args.discovery).parent),
            "matched_target_count": sum(1 for t in out_targets if t["scan_status"] == "matched"),
            "unmatched_target_count": sum(1 for t in out_targets if t["scan_status"] == "no_match"),
            "targets": out_targets,
        }
    )
    Path(args.output).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        f"wrote {args.output}: {result['matched_target_count']} matched / "
        f"{result['unmatched_target_count']} unmatched"
    )


if __name__ == "__main__":
    main()
