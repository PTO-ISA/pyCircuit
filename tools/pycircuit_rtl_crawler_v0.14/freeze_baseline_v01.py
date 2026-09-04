#!/usr/bin/env python3
"""Freeze a reproducible source/candidate discovery baseline.

The discovery crawler intentionally emits one row per target match.  This
utility keeps that raw evidence, creates one canonical row per
source/module/file candidate, and records the exact source commits used for
the baseline.  It does not update repositories or modify upstream RTL.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import shutil
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path


DISCOVERY_FILES = (
    "candidates_raw.csv",
    "candidates_raw.jsonl",
    "candidate_details.csv",
    "candidate_details.jsonl",
    "dependency_edges.csv",
    "file_metadata.csv",
    "module_inventory.csv",
    "unmatched_targets.csv",
)


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, rows: list[dict[str, str]], fields: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def unique_join(values: list[str]) -> str:
    return ";".join(sorted({value for value in values if value}))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def make_candidates(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    grouped: dict[tuple[str, str, str], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        key = (row.get("source_project", ""), row.get("module", ""), row.get("file", ""))
        grouped[key].append(row)

    candidates: list[dict[str, str]] = []
    for index, key in enumerate(sorted(grouped), start=1):
        matches = grouped[key]
        ranked = sorted(
            matches,
            key=lambda row: (
                -int(row.get("discovery_match_score", "0") or 0),
                row.get("target_id", ""),
                row.get("operation", ""),
            ),
        )
        primary = ranked[0]
        candidates.append(
            {
                "candidate_id": f"CAND-{index:04d}",
                "source_project": key[0],
                "module": key[1],
                "file": key[2],
                "repo_url": primary.get("repo_url", ""),
                "commit_sha": primary.get("commit_sha", ""),
                "branch": primary.get("branch", ""),
                "source_priority": primary.get("source_priority", ""),
                "primary_target_id": primary.get("target_id", ""),
                "target_ids": unique_join([row.get("target_id", "") for row in matches]),
                "operations": unique_join([row.get("operation", "") for row in matches]),
                "priorities": unique_join([row.get("priority", "") for row in matches]),
                "matched_keywords": unique_join(
                    [row.get("matched_keywords", "") for row in matches]
                ),
                "max_match_score": str(
                    max(int(row.get("discovery_match_score", "0") or 0) for row in matches)
                ),
                "validation_status": "not_run",
                "runtime_status": "discovery_candidate",
            }
        )
    return candidates


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sources", default="sources.expanded.v0.4.json")
    parser.add_argument("--source-manifest", default="build/source-cache-v0.4/materialize-manifest-v0.4.json")
    parser.add_argument("--targets", default="targets.v0.4.json")
    parser.add_argument("--discovery", default="build/discovery-v0.4/all-local-v0.4")
    parser.add_argument("--output", default="build/frozen-baseline-v0.1")
    args = parser.parse_args()

    root = Path.cwd()
    source_config = (root / args.sources).resolve()
    source_manifest = (root / args.source_manifest).resolve()
    targets = (root / args.targets).resolve()
    discovery = (root / args.discovery).resolve()
    output = (root / args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)

    for required in (source_config, source_manifest, targets, discovery / "candidates_raw.csv"):
        if not required.exists():
            raise SystemExit(f"missing required input: {required}")

    # Preserve the exact input artifacts alongside the derived canonical list.
    shutil.copy2(source_config, output / source_config.name)
    shutil.copy2(source_manifest, output / "source-manifest.json")
    shutil.copy2(targets, output / targets.name)
    for name in DISCOVERY_FILES:
        source = discovery / name
        if source.exists():
            shutil.copy2(source, output / name)

    raw_rows = read_csv(discovery / "candidates_raw.csv")
    candidates = make_candidates(raw_rows)
    candidate_fields = list(candidates[0]) if candidates else []
    write_csv(output / "candidates_frozen.csv", candidates, candidate_fields)
    with (output / "candidates_frozen.jsonl").open("w", encoding="utf-8") as stream:
        for candidate in candidates:
            stream.write(json.dumps(candidate, ensure_ascii=False, separators=(",", ":")) + "\n")

    source_data = json.loads(source_config.read_text(encoding="utf-8"))
    manifest_data = json.loads(source_manifest.read_text(encoding="utf-8"))
    scan_sources = [
        source for source in source_data.get("sources", [])
        if source.get("enabled", True) and source.get("scan", True)
    ]
    dependency_sources = [
        source for source in source_data.get("sources", [])
        if source.get("enabled", True) and not source.get("scan", True)
    ]
    by_project = defaultdict(int)
    for candidate in candidates:
        by_project[candidate["source_project"]] += 1
    matched_projects = sorted(by_project)
    unmatched_projects = sorted(
        source["project"] for source in scan_sources if source["project"] not in by_project
    )
    source_records = {
        record.get("project"): {
            "commit_sha": record.get("commit_sha", ""),
            "branch": record.get("branch", ""),
            "path": record.get("path", ""),
            "content_status": record.get("content_status", ""),
            "rtl_file_count": record.get("rtl_file_count", 0),
        }
        for record in manifest_data.get("sources", [])
    }
    frozen_artifacts = [
        source_config.name,
        "source-manifest.json",
        targets.name,
        "candidates_frozen.csv",
        "candidates_frozen.jsonl",
    ]
    artifact_hashes = {
        name: sha256(output / name)
        for name in frozen_artifacts
        if (output / name).exists()
    }
    baseline = {
        "schema": "pycircuit-rtl-freeze-v0.1",
        "frozen_at": datetime.now(timezone.utc).isoformat(),
        "source_config": source_config.name,
        "source_manifest": "source-manifest.json",
        "targets_config": targets.name,
        "discovery_input": str(discovery),
        "selection_policy": {
            "require_local_materialization": True,
            "dedup_key": ["source_project", "module", "file"],
            "license_gate": False,
            "license_recording": True,
        },
        "source_counts": {
            "configured": len(source_data.get("sources", [])),
            "scan": len(scan_sources),
            "dependency_only": len(dependency_sources),
            "materialized_manifest": len(manifest_data.get("sources", [])),
        },
        "candidate_counts": {
            "raw_matches": len(raw_rows),
            "unique_candidates": len(candidates),
            "matched_projects": len(matched_projects),
            "unmatched_scan_projects": len(unmatched_projects),
        },
        "candidate_distribution": dict(sorted(by_project.items())),
        "matched_projects": matched_projects,
        "unmatched_scan_projects": unmatched_projects,
        "source_records": source_records,
        "validation_input": "candidates_frozen.csv",
        "artifacts_sha256": artifact_hashes,
        "status": "frozen_discovery_baseline",
    }
    (output / "baseline_manifest.json").write_text(
        json.dumps(baseline, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(
        f"frozen {len(raw_rows)} raw matches -> {len(candidates)} unique candidates "
        f"from {len(source_data.get('sources', []))} configured sources at {output}"
    )


if __name__ == "__main__":
    main()
