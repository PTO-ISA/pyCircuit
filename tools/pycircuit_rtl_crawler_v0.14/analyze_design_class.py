#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import html
import json
from collections import defaultdict
from pathlib import Path


def to_int(v):
    if v is None or v == "":
        return None
    try:
        return int(float(v))
    except Exception:
        return None


def load_rows(path: Path):
    with path.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        r["n"] = to_int(r.get("n"))
        r["cells"] = to_int(r.get("cells"))
        r["depth"] = to_int(r.get("depth"))
        r["area"] = float(r["area"]) if r.get("area") not in ("", None, "None") else None
    return rows


def passing(r):
    return (
        r.get("closure") == "PASS"
        and r.get("build") == "PASS"
        and r.get("correctness") == "PASS"
        and r.get("synthesis") == "PASS"
        and r.get("cells") is not None
        and r.get("depth") is not None
    )


def pareto_for_group(rows):
    good = [r for r in rows if passing(r)]
    out = {}
    for a in good:
        dominated_by = []
        for b in good:
            if a is b:
                continue
            if (
                b["cells"] <= a["cells"]
                and b["depth"] <= a["depth"]
                and (b["cells"] < a["cells"] or b["depth"] < a["depth"])
            ):
                dominated_by.append(f"{b['project']}/{b['module']}")
        out[(a["project"], a["module"])] = {
            "pareto": len(dominated_by) == 0,
            "dominated_by": dominated_by,
        }
    return out


def pct(a, b):
    if a is None or b in (None, 0):
        return None
    return (a - b) / b * 100.0


def analyze(rows):
    by_n = defaultdict(list)
    by_candidate = defaultdict(list)

    for r in rows:
        by_n[r["n"]].append(r)
        by_candidate[(r["project"], r["module"])].append(r)

    # Pareto per N.
    for n, group in by_n.items():
        p = pareto_for_group(group)
        for r in group:
            info = p.get((r["project"], r["module"]), {})
            r["pareto"] = bool(info.get("pareto", False))
            r["dominated_by"] = ";".join(info.get("dominated_by", []))

    scaling = []
    for (project, module), group in by_candidate.items():
        good = sorted([r for r in group if passing(r)], key=lambda x: x["n"])
        if not good:
            continue
        first, last = good[0], good[-1]
        scaling.append({
            "project": project,
            "module": module,
            "n_min": first["n"],
            "n_max": last["n"],
            "cells_min": first["cells"],
            "cells_max": last["cells"],
            "depth_min": first["depth"],
            "depth_max": last["depth"],
            "cells_growth_x": round(last["cells"]/first["cells"], 4) if first["cells"] else None,
            "depth_growth_x": round(last["depth"]/first["depth"], 4) if first["depth"] else None,
        })

    # Best values per N and relative overheads.
    point_summary = []
    for n in sorted(k for k in by_n if k is not None):
        good = [r for r in by_n[n] if passing(r)]
        if not good:
            continue
        min_cells = min(r["cells"] for r in good)
        min_depth = min(r["depth"] for r in good)
        for r in good:
            point_summary.append({
                "n": n,
                "project": r["project"],
                "module": r["module"],
                "cells": r["cells"],
                "depth": r["depth"],
                "cells_over_best_pct": round(pct(r["cells"], min_cells), 2),
                "depth_over_best_pct": round(pct(r["depth"], min_depth), 2),
                "pareto": r["pareto"],
                "dominated_by": r["dominated_by"],
            })

    return {
        "rows": rows,
        "scaling": scaling,
        "points": point_summary,
    }


def write_csv(path, rows, fields):
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow({k: r.get(k, "") for k in fields})


def make_html(a, out: Path):
    point_rows = []
    for r in sorted(a["points"], key=lambda x: (x["n"], x["cells"], x["depth"])):
        point_rows.append(
            "<tr>"
            f"<td>{r['n']}</td>"
            f"<td>{html.escape(r['project'])}</td>"
            f"<td>{html.escape(r['module'])}</td>"
            f"<td>{r['cells']}</td>"
            f"<td>{r['depth']}</td>"
            f"<td>{r['cells_over_best_pct']:.2f}%</td>"
            f"<td>{r['depth_over_best_pct']:.2f}%</td>"
            f"<td>{'YES' if r['pareto'] else ''}</td>"
            f"<td>{html.escape(r['dominated_by'])}</td>"
            "</tr>"
        )

    scale_rows = []
    for r in a["scaling"]:
        scale_rows.append(
            "<tr>"
            f"<td>{html.escape(r['project'])}</td>"
            f"<td>{html.escape(r['module'])}</td>"
            f"<td>{r['n_min']}→{r['n_max']}</td>"
            f"<td>{r['cells_min']}→{r['cells_max']}</td>"
            f"<td>{r['cells_growth_x']:.2f}×</td>"
            f"<td>{r['depth_min']}→{r['depth_max']}</td>"
            f"<td>{r['depth_growth_x']:.2f}×</td>"
            "</tr>"
        )

    # Derive narrative observations conservatively from measured data.
    by_n = defaultdict(list)
    for r in a["points"]:
        by_n[r["n"]].append(r)

    observations = []
    for n in sorted(by_n):
        ps = [r for r in by_n[n] if r["pareto"]]
        names = ", ".join(f"{r['project']}/{r['module']}" for r in ps)
        observations.append(
            f"<li><b>N={n}</b>: Pareto frontier = {html.escape(names)}</li>"
        )

    doc = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>DF-09 Scaling & Pareto Analysis</title>
<style>
body{{font-family:Arial,sans-serif;margin:32px;color:#1f2937;line-height:1.55}}
h1,h2{{color:#111827}}
table{{border-collapse:collapse;width:100%;margin:16px 0 28px}}
th,td{{border:1px solid #d1d5db;padding:8px;text-align:left;font-size:14px}}
th{{background:#f3f4f6}}
.note{{background:#f8fafc;border-left:4px solid #64748b;padding:12px 14px}}
</style></head><body>
<h1>pyCircuit DF-09 · Scaling & Pareto Analysis</h1>

<div class="note">
<b>Metric semantics:</b> cells are Yosys generic cell counts and depth is the
current logic-topology depth proxy. These are <b>not</b> technology-mapped area
or Fmax. Results include the canonical integration adapter and should therefore
be interpreted as <b>Canonical Integration QoR</b>.
</div>

<h2>Pareto Frontier by NumIn</h2>
<ul>
{''.join(observations)}
</ul>

<h2>Per-Design-Point Comparison</h2>
<table>
<thead><tr>
<th>N</th><th>Project</th><th>Module</th><th>Cells</th><th>Depth</th>
<th>Cells vs best</th><th>Depth vs best</th><th>Pareto</th><th>Dominated by</th>
</tr></thead>
<tbody>{''.join(point_rows)}</tbody>
</table>

<h2>Scaling Summary</h2>
<table>
<thead><tr>
<th>Project</th><th>Module</th><th>N range</th>
<th>Cells</th><th>Cell growth</th><th>Depth</th><th>Depth growth</th>
</tr></thead>
<tbody>{''.join(scale_rows)}</tbody>
</table>

<h2>Interpretation Guardrails</h2>
<p>
Do not infer final ASIC superiority from generic cell count alone. The next
technology-aware stage should map all candidates to the same Liberty library
and timing constraints, then compare mapped area and critical-path delay/Fmax.
</p>
</body></html>"""
    out.write_text(doc, encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(description="Analyze design-class scaling and Pareto results.")
    ap.add_argument("comparison_csv", type=Path)
    ap.add_argument("--outdir", type=Path, default=None)
    args = ap.parse_args()

    csv_path = args.comparison_csv.resolve()
    if not csv_path.exists():
        raise SystemExit(f"Not found: {csv_path}")

    outdir = args.outdir.resolve() if args.outdir else csv_path.parent / "analysis"
    outdir.mkdir(parents=True, exist_ok=True)

    rows = load_rows(csv_path)
    a = analyze(rows)

    write_csv(
        outdir / "pareto_points.csv",
        a["points"],
        ["n","project","module","cells","depth","cells_over_best_pct",
         "depth_over_best_pct","pareto","dominated_by"],
    )
    write_csv(
        outdir / "scaling_summary.csv",
        a["scaling"],
        ["project","module","n_min","n_max","cells_min","cells_max",
         "cells_growth_x","depth_min","depth_max","depth_growth_x"],
    )
    (outdir / "analysis.json").write_text(
        json.dumps(a, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    make_html(a, outdir / "analysis.html")

    print("=== Design-Class Scaling/Pareto Analyzer ===")
    print("input  :", csv_path)
    print("points :", len(a["points"]))
    print("html   :", outdir / "analysis.html")
    print("pareto :", outdir / "pareto_points.csv")
    print("scale  :", outdir / "scaling_summary.csv")


if __name__ == "__main__":
    main()
