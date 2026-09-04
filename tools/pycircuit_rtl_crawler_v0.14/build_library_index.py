#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import html
import json
from pathlib import Path


def main():
    ap = argparse.ArgumentParser(
        description="Build a multi-design-class Runtime Library index."
    )
    ap.add_argument(
        "--library-root",
        type=Path,
        default=Path("runtime_design_library"),
    )
    args = ap.parse_args()

    catalogs = []
    for p in sorted(args.library_root.glob("*/*/runtime_catalog.json")):
        try:
            d = json.loads(p.read_text(encoding="utf-8"))
        except Exception:
            continue
        catalogs.append((p, d))

    rows = []
    for p, d in catalogs:
        dc = d.get("design_class", {})
        rows.append({
            "class_id": dc.get("id", ""),
            "family": dc.get("family", ""),
            "operation": dc.get("operation", ""),
            "profile": d.get("profile", ""),
            "records": d.get("record_count", 0),
            "valid_records": d.get("valid_record_count", 0),
            "selection_complete": d.get(
                "selection_complete_for_all_configs", False
            ),
            "catalog": str(p),
        })

    out_json = args.library_root / "library_index.json"
    out_csv = args.library_root / "library_index.csv"
    out_html = args.library_root / "library_index.html"

    out_json.write_text(
        json.dumps({"design_classes": rows}, indent=2) + "\n",
        encoding="utf-8",
    )

    with out_csv.open("w", newline="", encoding="utf-8") as f:
        fields = [
            "class_id", "family", "operation", "profile",
            "records", "valid_records", "selection_complete", "catalog"
        ]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    trs = []
    for r in rows:
        trs.append(
            "<tr>"
            f"<td>{html.escape(r['class_id'])}</td>"
            f"<td>{html.escape(r['family'])}</td>"
            f"<td>{html.escape(r['operation'])}</td>"
            f"<td>{html.escape(r['profile'])}</td>"
            f"<td>{r['records']}</td>"
            f"<td>{r['valid_records']}</td>"
            f"<td>{'YES' if r['selection_complete'] else 'NO'}</td>"
            "</tr>"
        )

    out_html.write_text(
        """<!doctype html><html><head><meta charset="utf-8">
<style>
body{font-family:Arial,sans-serif;margin:32px;color:#1f2937}
table{border-collapse:collapse;width:100%}
th,td{border:1px solid #d1d5db;padding:8px;text-align:left}
th{background:#f3f4f6}
</style></head><body>
<h1>pyCircuit Runtime Hardware Design Library</h1>
<table><thead><tr>
<th>Design Class</th><th>Family</th><th>Operation</th><th>Profile</th>
<th>Records</th><th>Valid</th><th>Selection Complete</th>
</tr></thead><tbody>"""
        + "".join(trs)
        + "</tbody></table></body></html>",
        encoding="utf-8",
    )

    print("=== Runtime Library Index ===")
    print("design classes:", len(rows))
    print("json :", out_json)
    print("csv  :", out_csv)
    print("html :", out_html)


if __name__ == "__main__":
    main()
