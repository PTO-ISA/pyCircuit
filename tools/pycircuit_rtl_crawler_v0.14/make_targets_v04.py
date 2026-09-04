#!/usr/bin/env python3
"""Create the v0.4 target manifest without mutating the v0.3 input.

The v0.4 manifest keeps the 236 canonical search targets and adds explicit
state/provenance fields so discovery, local scanning, validation, and runtime
acceptance cannot be conflated.
"""
from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--targets", default="targets.json")
    ap.add_argument("--sources", default="sources.expanded.completed.v0.3.json")
    ap.add_argument("--output", default="targets.v0.4.json")
    args = ap.parse_args()

    targets_cfg = load(Path(args.targets))
    sources_cfg = load(Path(args.sources))
    sources = [s for s in sources_cfg.get("sources", []) if s.get("enabled", True)]
    source_names = [s["project"] for s in sources]

    # Keep the source map deterministic and conservative: a source is listed
    # for a target when at least one declared family overlaps.  This is only a
    # search hint; the crawler still performs module/keyword matching locally.
    family_map = {
        s["project"]: {str(f).lower() for f in s.get("families", [])}
        for s in sources
    }
    out_targets = []
    for original in targets_cfg.get("targets", []):
        target = dict(original)
        target_family = str(target.get("family", "")).lower()
        target_domain = str(target.get("domain", "")).lower()
        hints = {target_family, target_domain}
        candidates = []
        for project in source_names:
            if any(h and (h in family_map[project] or h in " ".join(family_map[project])) for h in hints):
                candidates.append(project)
        # Do not leave a target with no search roots merely because the family
        # labels differ between the requirements document and source catalog.
        if not candidates:
            candidates = list(source_names)

        target.update(
            {
                "candidate_source_projects": candidates,
                "local_source_required": True,
                "clone_status": "pending",
                "scan_status": "pending",
                "validation_status": "not_run",
                "runtime_status": "search_candidate_only",
                "license_gate": False,
            }
        )
        out_targets.append(target)

    result = {
        "schema": "pycircuit-rtl-targets-v0.4",
        "source_document": targets_cfg.get("source_document", ""),
        "generated_from": f"targets.json + {Path(args.sources).name}",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "target_count": len(out_targets),
        "source_count": len(source_names),
        "source_config": Path(args.sources).name,
        "local_source_required": True,
        "selection_policy": {
            "license_gate": False,
            "license_recording": True,
            "require_clone_before_match": True,
            "require_structural_rtl_match": True,
            "runtime_acceptance_requires_verification": True,
        },
        "enabled_source_projects": source_names,
        "targets": out_targets,
    }
    Path(args.output).write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(f"wrote {args.output}: {len(out_targets)} targets, {len(source_names)} sources")


if __name__ == "__main__":
    main()
