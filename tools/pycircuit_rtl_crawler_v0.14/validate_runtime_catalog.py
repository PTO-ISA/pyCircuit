#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

REQUIRED_QOR = ("mapped_area", "critical_delay_ns", "fmax_proxy_mhz")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("catalog", type=Path)
    ap.add_argument(
        "--require-complete-selection",
        action="store_true",
        help="Fail unless every config has all expected candidates valid."
    )
    args = ap.parse_args()

    d = json.loads(args.catalog.read_text(encoding="utf-8"))
    errors = []
    ids = set()

    for r in d.get("records", []):
        rid = r.get("record_id", "")
        if not rid or rid in ids:
            errors.append(f"bad/duplicate record_id: {rid}")
        ids.add(rid)

        for gate in ("closure", "build", "correctness", "synthesis", "timing"):
            if r.get("gates", {}).get(gate) != "PASS":
                errors.append(f"{rid}: {gate} != PASS")

        for k in REQUIRED_QOR:
            if r.get("qor", {}).get(k) is None:
                errors.append(f"{rid}: missing {k}")

        cfg = r.get("configuration", {})
        if not cfg.get("config"):
            errors.append(f"{rid}: missing configuration.config")

    if args.require_complete_selection:
        for rec in d.get("recommendations", []):
            if not rec.get("selection_complete"):
                errors.append(
                    f"{rec.get('config')}: selection incomplete "
                    f"({rec.get('valid_candidates')}/"
                    f"{rec.get('expected_candidates')})"
                )

    if errors:
        print("CATALOG_VALIDATE_FAIL")
        for e in errors:
            print(" -", e)
        raise SystemExit(2)

    print("CATALOG_VALIDATE_PASS")
    print("records:", len(ids))
    print(
        "all selections complete:",
        d.get("selection_complete_for_all_configs"),
    )


if __name__ == "__main__":
    main()
