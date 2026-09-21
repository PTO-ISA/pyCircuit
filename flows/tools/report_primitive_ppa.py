#!/usr/bin/env python3
"""Report structural PPA metadata for qualified RTL primitive implementations.

This tool is report-only. It never gates a build and it encodes no regression
threshold: the framework records structural estimates as evidence first and adds
thresholds only after a stable baseline exists.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

_METADATA_FIELDS = (
    "latency_cycles",
    "initiation_interval",
    "pipeline_depth",
    "banks",
    "depth_entries",
    "storage",
    "structural_estimate",
)


def build_report(catalog_path: Path) -> tuple[dict[str, object] | None, str | None]:
    try:
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return None, f"cannot load {catalog_path}: {exc}"
    if catalog.get("schema") != "pyc-rtl-catalog-v1":
        return None, "RTL catalog must declare schema pyc-rtl-catalog-v1"
    report: dict[str, object] = {
        "schema": "pyc-primitive-ppa-report-v1",
        "gating": False,
        "note": (
            "Structural estimates only. No regression threshold is applied; "
            "thresholds follow only after a stable baseline exists."
        ),
        "implementations": [],
    }
    implementations: list[dict[str, object]] = []
    for entry in catalog.get("implementations", []):
        metadata = entry.get("metadata")
        if not isinstance(metadata, dict):
            return None, f"{entry.get('implementation_id')} lacks metadata"
        missing = [field for field in _METADATA_FIELDS if field not in metadata]
        if missing:
            return (
                None,
                f"{entry.get('implementation_id')} metadata is missing {missing}",
            )
        row: dict[str, object] = {
            "semantic_id": entry.get("semantic_id"),
            "implementation_id": entry.get("implementation_id"),
            "module": entry.get("module"),
            "effect_class": entry.get("effect_class"),
            "selection_priority": entry.get("selection_priority"),
            "min_width": entry.get("min_width"),
            "max_width": entry.get("max_width"),
        }
        for field in _METADATA_FIELDS:
            row[field] = metadata[field]
        implementations.append(row)
    implementations.sort(key=lambda row: str(row["implementation_id"]))
    report["implementations"] = implementations
    return report, None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--catalog", default="library/verilog/rtl_catalog.json", type=Path
    )
    parser.add_argument("--out", type=Path)
    arguments = parser.parse_args()
    report, error = build_report(arguments.catalog)
    if error is not None or report is None:
        print(f"error: {error}", file=sys.stderr)
        return 1
    text = json.dumps(report, indent=2, sort_keys=False) + "\n"
    if arguments.out is not None:
        arguments.out.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
