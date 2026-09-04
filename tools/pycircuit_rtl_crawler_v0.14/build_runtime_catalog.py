#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import hashlib
import html
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import yaml


def read_csv(path: Path):
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def read_json(path: Path, default=None):
    if not path.exists():
        return {} if default is None else default
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {} if default is None else default


def as_int(v):
    if v in (None, "", "None"):
        return None
    try:
        return int(float(v))
    except Exception:
        return None


def as_float(v):
    if v in (None, "", "None"):
        return None
    try:
        return float(v)
    except Exception:
        return None


def sha256(path: Path):
    if not path or not path.exists():
        return ""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def source_repo_map(path: Path):
    if not path.exists():
        return {}
    data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    return {
        x.get("project", ""): x.get("repo", "")
        for x in data.get("sources", [])
    }


def design_class_spec(spec_path: Path, class_id: str):
    data = yaml.safe_load(spec_path.read_text(encoding="utf-8")) or {}
    dc = data.get("design_classes", {}).get(class_id, {})
    cmap = {
        (c.get("project", ""), c.get("module", "")): c
        for c in dc.get("candidates", [])
    }
    profile_cfg = {
        str(c.get("name", "")): c
        for c in dc.get("profiles", {}).get("scaling", [])
    }
    return dc, cmap, profile_cfg


def manifest_commit(candidate_root: Path | None, project: str, module: str):
    if not candidate_root:
        return ""
    p = candidate_root / project / module / "manifest.json"
    obj = read_json(p)
    for key in ("commit", "source_commit", "repo_commit", "git_commit", "sha"):
        if obj.get(key):
            return str(obj[key])
    src = obj.get("source", {}) if isinstance(obj.get("source"), dict) else {}
    for key in ("commit", "sha"):
        if src.get(key):
            return str(src[key])
    return ""


def config_payload(row: dict, profile_cfg: dict):
    cfg_name = row.get("config", "")
    cfg = {
        "profile": row.get("profile", ""),
        "config": cfg_name,
    }

    # Prefer explicit benchmark row fields.
    for key in ("n", "data_width", "capacity"):
        value = as_int(row.get(key))
        if value is not None:
            cfg[key] = value

    # Merge other class-specific profile parameters when available.
    src = profile_cfg.get(cfg_name, {})
    for key, value in src.items():
        if key == "name" or key in cfg:
            continue
        if isinstance(value, (str, int, float, bool)) or value is None:
            cfg[key] = value
    return cfg


def valid_record(r):
    return (
        r["gates"]["closure"] == "PASS"
        and r["gates"]["build"] == "PASS"
        and r["gates"]["correctness"] == "PASS"
        and r["gates"]["synthesis"] == "PASS"
        and r["gates"]["timing"] == "PASS"
        and r["qor"]["mapped_area"] is not None
        and r["qor"]["critical_delay_ns"] is not None
        and r["qor"]["fmax_proxy_mhz"] is not None
    )


def apply_pareto(group):
    for r in group:
        r["selection"]["area_timing_pareto"] = False
        r["selection"]["dominated_by"] = []

    good = [r for r in group if valid_record(r)]
    for a in good:
        dom = []
        aa = a["qor"]["mapped_area"]
        ad = a["qor"]["critical_delay_ns"]
        for b in good:
            if a is b:
                continue
            ba = b["qor"]["mapped_area"]
            bd = b["qor"]["critical_delay_ns"]
            if ba <= aa and bd <= ad and (ba < aa or bd < ad):
                dom.append(b["record_id"])
        a["selection"]["dominated_by"] = dom
        a["selection"]["area_timing_pareto"] = not dom


def recommendations(records, expected_candidates: int):
    by_cfg = defaultdict(list)
    for r in records:
        by_cfg[r["configuration"]["config"]].append(r)

    out = []
    for cfg_name in sorted(by_cfg):
        group = by_cfg[cfg_name]
        good = [r for r in group if valid_record(r)]
        complete = (
            len(group) == expected_candidates
            and len(good) == expected_candidates
        )

        entry = {
            "config": cfg_name,
            "selection_complete": complete,
            "expected_candidates": expected_candidates,
            "observed_candidates": len(group),
            "valid_candidates": len(good),
            "area_winner": None,
            "timing_winner": None,
            "pareto_set": [],
            "timing_winner_area_overhead_vs_area_winner_pct": None,
            "timing_winner_delay_reduction_vs_area_winner_pct": None,
        }

        if good:
            area_w = min(good, key=lambda r: r["qor"]["mapped_area"])
            time_w = min(good, key=lambda r: r["qor"]["critical_delay_ns"])
            pset = [
                r for r in good
                if r["selection"]["area_timing_pareto"]
            ]

            area_base = area_w["qor"]["mapped_area"]
            delay_base = area_w["qor"]["critical_delay_ns"]
            timing_area = time_w["qor"]["mapped_area"]
            timing_delay = time_w["qor"]["critical_delay_ns"]

            entry.update({
                "area_winner": area_w["record_id"],
                "timing_winner": time_w["record_id"],
                "pareto_set": [r["record_id"] for r in pset],
                "timing_winner_area_overhead_vs_area_winner_pct": round(
                    (timing_area - area_base) / area_base * 100.0, 4
                ) if area_base else None,
                "timing_winner_delay_reduction_vs_area_winner_pct": round(
                    (delay_base - timing_delay) / delay_base * 100.0, 4
                ) if delay_base else None,
            })
        out.append(entry)
    return out


def write_html(catalog, path: Path):
    def fmt(v, digits=4):
        if v is None:
            return ""
        if isinstance(v, float):
            return f"{v:.{digits}f}"
        return str(v)

    rows = []
    for r in sorted(
        catalog["records"],
        key=lambda x: (
            x["configuration"]["config"],
            x["qor"]["mapped_area"] if x["qor"]["mapped_area"] is not None else 1e30,
        ),
    ):
        q = r["qor"]
        s = r["selection"]
        rows.append(
            "<tr>"
            f"<td>{html.escape(r['configuration']['config'])}</td>"
            f"<td>{html.escape(r['source']['project'])}</td>"
            f"<td>{html.escape(r['source']['module'])}</td>"
            f"<td>{fmt(q['generic_cells'], 0)}</td>"
            f"<td>{fmt(q['logic_depth'], 0)}</td>"
            f"<td>{fmt(q['mapped_area'])}</td>"
            f"<td>{fmt(q['critical_delay_ns'])}</td>"
            f"<td>{fmt(q['fmax_proxy_mhz'], 2)}</td>"
            f"<td>{'YES' if s['area_timing_pareto'] else ''}</td>"
            "</tr>"
        )

    rec_rows = []
    for r in catalog["recommendations"]:
        status = "COMPLETE" if r["selection_complete"] else "INCOMPLETE"
        rec_rows.append(
            "<tr>"
            f"<td>{html.escape(r['config'])}</td>"
            f"<td>{status}</td>"
            f"<td>{html.escape(r['area_winner'] or '')}</td>"
            f"<td>{html.escape(r['timing_winner'] or '')}</td>"
            f"<td>{html.escape(', '.join(r['pareto_set']))}</td>"
            "</tr>"
        )

    doc = f"""<!doctype html>
<html><head><meta charset="utf-8">
<title>pyCircuit Runtime Design Catalog</title>
<style>
body{{font-family:Arial,sans-serif;margin:32px;color:#1f2937;line-height:1.5}}
table{{border-collapse:collapse;width:100%;font-size:13px;margin:12px 0 28px}}
th,td{{border:1px solid #d1d5db;padding:8px;text-align:left}}
th{{background:#f3f4f6}}
.note{{background:#f8fafc;border-left:4px solid #2563eb;padding:12px 14px}}
</style></head><body>
<h1>pyCircuit Runtime Design Catalog · {html.escape(catalog['design_class']['id'])}</h1>
<div class="note">
Records are keyed by Design Class × Source Implementation × Canonical Configuration.
Mapped area and Fmax are pre-layout benchmark evidence, not signoff PPA.
</div>
<h2>Design Records</h2>
<table><thead><tr>
<th>Config</th><th>Project</th><th>Module</th>
<th>Generic Cells</th><th>Logic Depth</th><th>Mapped Area</th>
<th>Delay ns</th><th>Fmax Proxy MHz</th><th>Pareto</th>
</tr></thead><tbody>{''.join(rows)}</tbody></table>

<h2>Runtime Selection View</h2>
<table><thead><tr>
<th>Config</th><th>Selection Status</th><th>Area Winner</th>
<th>Timing Winner</th><th>Pareto Set</th>
</tr></thead><tbody>{''.join(rec_rows)}</tbody></table>
</body></html>"""
    path.write_text(doc, encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(
        description="Build generic pyCircuit Runtime Design Catalog."
    )
    ap.add_argument("--class-id", required=True)
    ap.add_argument("--profile", default="scaling")
    ap.add_argument(
        "--results-root",
        type=Path,
        default=Path("design_class_results"),
    )
    ap.add_argument(
        "--specs",
        type=Path,
        default=Path("design_class_specs.yaml"),
    )
    ap.add_argument(
        "--sources",
        type=Path,
        default=Path("sources.yaml"),
    )
    ap.add_argument("--candidate-root", type=Path, default=None)
    ap.add_argument("--liberty", type=Path, default=None)
    ap.add_argument(
        "--out-root",
        type=Path,
        default=Path("runtime_design_library"),
    )
    args = ap.parse_args()

    base = args.results_root / args.class_id / args.profile
    comparison_csv = base / "comparison.csv"
    timing_csv = base / "timing_analysis" / "timing_summary.csv"
    comparison_report = read_json(base / "comparison_report.json")
    timing_report = read_json(base / "timing_analysis" / "timing_report.json")

    if not comparison_csv.exists():
        raise SystemExit(f"Missing: {comparison_csv}")
    if not timing_csv.exists():
        raise SystemExit(f"Missing: {timing_csv}")

    dc, cand_specs, profile_cfg = design_class_spec(args.specs, args.class_id)
    if not dc:
        raise SystemExit(f"Unknown design class: {args.class_id}")

    # Rebuild profile mapping for the selected profile.
    all_specs = yaml.safe_load(args.specs.read_text(encoding="utf-8")) or {}
    selected_profile_cfg = {
        str(c.get("name", "")): c
        for c in all_specs["design_classes"][args.class_id]
        .get("profiles", {}).get(args.profile, [])
    }

    repos = source_repo_map(args.sources)
    comp_rows = read_csv(comparison_csv)
    time_rows = read_csv(timing_csv)
    tmap = {
        (r["project"], r["module"], r["config"]): r
        for r in time_rows
    }

    liberty_path = None
    if args.liberty:
        liberty_path = args.liberty.resolve()
    elif timing_report.get("liberty"):
        liberty_path = Path(timing_report["liberty"])

    lib_sha = sha256(liberty_path) if liberty_path else ""

    records = []
    for c in comp_rows:
        key = (c["project"], c["module"], c["config"])
        t = tmap.get(key, {})
        spec = cand_specs.get((c["project"], c["module"]), {})
        cfg = config_payload(
            {**c, "profile": args.profile},
            selected_profile_cfg,
        )
        rid = f"{args.class_id}:{c['project']}:{c['module']}:{c['config']}"

        record = {
            "schema_version": "1.1",
            "record_id": rid,
            "design_class": {
                "id": args.class_id,
                "family": dc.get("family", ""),
                "operation": dc.get("operation", ""),
            },
            "source": {
                "project": c["project"],
                "module": c["module"],
                "repo": repos.get(c["project"], ""),
                "commit": manifest_commit(
                    args.candidate_root, c["project"], c["module"]
                ),
                "source_hint": spec.get("source_hint", ""),
                "adapter": c.get("adapter", spec.get("adapter", "")),
            },
            "configuration": cfg,
            "gates": {
                "closure": c.get("closure", ""),
                "build": c.get("build", ""),
                "correctness": c.get("correctness", ""),
                "synthesis": c.get("synthesis", ""),
                "timing": t.get("timing_status", ""),
            },
            "qor": {
                "generic_cells": as_int(c.get("cells")),
                "logic_depth": as_int(c.get("depth")),
                "mapped_area": as_float(
                    t.get("mapped_area", c.get("area"))
                ),
                "critical_delay_ns": as_float(
                    t.get("critical_delay_ns")
                ),
                "fmax_proxy_mhz": as_float(
                    t.get("fmax_proxy_mhz")
                ),
            },
            "selection": {
                "area_timing_pareto": False,
                "dominated_by": [],
            },
            "provenance": {
                "frontend": comparison_report.get("frontend", ""),
                "yosys_version": comparison_report.get(
                    "yosys_version", ""
                ),
                "sta_version": timing_report.get(
                    "sta_version", ""
                ),
                "liberty": str(liberty_path) if liberty_path else "",
                "liberty_sha256": lib_sha,
                "timing_contract": timing_report.get(
                    "timing_contract", {}
                ),
                "timing_semantics": timing_report.get(
                    "timing_semantics", {}
                ),
                "qor_formal_sanitized": True,
                "comparison_csv": str(comparison_csv),
                "timing_csv": str(timing_csv),
            },
        }
        records.append(record)

    by_cfg = defaultdict(list)
    for r in records:
        by_cfg[r["configuration"]["config"]].append(r)
    for group in by_cfg.values():
        apply_pareto(group)

    expected_candidates = len(dc.get("candidates", []))
    recs = recommendations(records, expected_candidates)

    out = args.out_root / args.class_id / args.profile
    out.mkdir(parents=True, exist_ok=True)

    catalog = {
        "schema_version": "1.1",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "design_class": {
            "id": args.class_id,
            "family": dc.get("family", ""),
            "operation": dc.get("operation", ""),
            "canonical_contract": dc.get("canonical_contract", {}),
            "timing_contract": dc.get("timing_contract", {}),
        },
        "profile": args.profile,
        "expected_candidate_count": expected_candidates,
        "record_count": len(records),
        "valid_record_count": sum(valid_record(r) for r in records),
        "selection_complete_for_all_configs": all(
            r["selection_complete"] for r in recs
        ),
        "recommendations": recs,
        "records": records,
    }

    (out / "runtime_catalog.json").write_text(
        json.dumps(catalog, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    with (out / "design_records.jsonl").open("w", encoding="utf-8") as f:
        for r in records:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")

    fields = [
        "record_id", "config", "project", "module",
        "n", "data_width", "capacity",
        "generic_cells", "logic_depth", "mapped_area",
        "critical_delay_ns", "fmax_proxy_mhz", "pareto",
    ]
    with (out / "design_records.csv").open(
        "w", newline="", encoding="utf-8"
    ) as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in records:
            cfg = r["configuration"]
            q = r["qor"]
            w.writerow({
                "record_id": r["record_id"],
                "config": cfg.get("config", ""),
                "project": r["source"]["project"],
                "module": r["source"]["module"],
                "n": cfg.get("n", ""),
                "data_width": cfg.get("data_width", ""),
                "capacity": cfg.get("capacity", ""),
                "generic_cells": q["generic_cells"],
                "logic_depth": q["logic_depth"],
                "mapped_area": q["mapped_area"],
                "critical_delay_ns": q["critical_delay_ns"],
                "fmax_proxy_mhz": q["fmax_proxy_mhz"],
                "pareto": r["selection"]["area_timing_pareto"],
            })

    write_html(catalog, out / "runtime_catalog.html")

    print("=== pyCircuit Generic Runtime Design Catalog v0.13 ===")
    print("class        :", args.class_id)
    print("profile      :", args.profile)
    print("records      :", len(records))
    print("valid        :", catalog["valid_record_count"])
    print("all complete :", catalog["selection_complete_for_all_configs"])
    print("json         :", out / "runtime_catalog.json")
    print("jsonl        :", out / "design_records.jsonl")
    print("csv          :", out / "design_records.csv")
    print("html         :", out / "runtime_catalog.html")
    print()
    for x in recs:
        status = "COMPLETE" if x["selection_complete"] else "INCOMPLETE"
        print(
            f"{x['config']} | {status} "
            f"| area={x['area_winner']} "
            f"| timing={x['timing_winner']} "
            f"| pareto={','.join(x['pareto_set'])}"
        )


if __name__ == "__main__":
    main()
