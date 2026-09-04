#!/usr/bin/env python3
from __future__ import annotations
import argparse
import csv
import html
import json
from collections import defaultdict
from pathlib import Path

def num(v):
    if v in ("", None, "None"):
        return None
    try:
        return float(v)
    except Exception:
        return None

def main():
    ap = argparse.ArgumentParser(description="Analyze mapped-area design-class benchmark.")
    ap.add_argument("comparison_csv", type=Path)
    ap.add_argument("--outdir", type=Path, default=None)
    args = ap.parse_args()

    inp = args.comparison_csv.resolve()
    if not inp.exists():
        raise SystemExit(f"Not found: {inp}")

    out = args.outdir.resolve() if args.outdir else inp.parent / "mapped_area_analysis"
    out.mkdir(parents=True, exist_ok=True)

    with inp.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    good = []
    for r in rows:
        r["n"] = int(r["n"])
        r["area"] = num(r.get("area"))
        if (
            r.get("closure") == "PASS"
            and r.get("build") == "PASS"
            and r.get("correctness") == "PASS"
            and r.get("synthesis") == "PASS"
            and r["area"] is not None
        ):
            good.append(r)

    by_n = defaultdict(list)
    for r in good:
        by_n[r["n"]].append(r)

    summaries = []
    for n in sorted(by_n):
        group = by_n[n]
        best = min(r["area"] for r in group)
        for r in sorted(group, key=lambda x: x["area"]):
            summaries.append({
                "n": n,
                "project": r["project"],
                "module": r["module"],
                "mapped_area": r["area"],
                "area_vs_best_pct": (r["area"] - best) / best * 100.0 if best else 0.0,
                "best_area": r["area"] == best,
            })

    with (out / "mapped_area_summary.csv").open("w", newline="", encoding="utf-8") as f:
        fields = ["n","project","module","mapped_area","area_vs_best_pct","best_area"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(summaries)

    (out / "mapped_area_summary.json").write_text(
        json.dumps(summaries, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8"
    )

    trs = []
    for r in summaries:
        trs.append(
            "<tr>"
            f"<td>{r['n']}</td>"
            f"<td>{html.escape(r['project'])}</td>"
            f"<td>{html.escape(r['module'])}</td>"
            f"<td>{r['mapped_area']:.4f}</td>"
            f"<td>{r['area_vs_best_pct']:.2f}%</td>"
            f"<td>{'YES' if r['best_area'] else ''}</td>"
            "</tr>"
        )

    doc = f"""<!doctype html>
<html><head><meta charset="utf-8">
<title>pyCircuit Technology-Aware Area Analysis</title>
<style>
body{{font-family:Arial,sans-serif;margin:32px;color:#1f2937;line-height:1.55}}
table{{border-collapse:collapse;width:100%}}
th,td{{border:1px solid #d1d5db;padding:8px;text-align:left}}
th{{background:#f3f4f6}}
.note{{background:#f8fafc;border-left:4px solid #2563eb;padding:12px 14px;margin:16px 0}}
</style></head><body>
<h1>DF-09 Technology-Aware Mapped Area</h1>
<div class="note">
All candidates must use the <b>same Liberty file</b>. The area value is the
sum of Liberty cell areas after Yosys/ABC technology mapping. This is
pre-layout mapped area, not post-place-and-route silicon area.
</div>
<table>
<thead><tr><th>N</th><th>Project</th><th>Module</th><th>Mapped Area</th>
<th>Area vs best</th><th>Best</th></tr></thead>
<tbody>{''.join(trs)}</tbody>
</table>
</body></html>"""
    (out / "mapped_area.html").write_text(doc, encoding="utf-8")

    print("=== Technology-Aware Area Analyzer ===")
    print("valid design points :", len(summaries))
    print("csv :", out / "mapped_area_summary.csv")
    print("json:", out / "mapped_area_summary.json")
    print("html:", out / "mapped_area.html")

if __name__ == "__main__":
    main()
