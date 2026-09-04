#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import html
import json
import subprocess
import sys
import time
import os
import shutil
from pathlib import Path

import yaml


PASSLIKE = {"PASS", "PARTIAL"}
STAGES = ("build", "correctness", "stateful", "synthesis")


def run_cmd(cmd, cwd: Path, env=None):
    t0 = time.time()
    p = subprocess.run(
        cmd,
        cwd=str(cwd),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    return {
        "cmd": cmd,
        "returncode": p.returncode,
        "seconds": round(time.time() - t0, 4),
        "stdout": p.stdout or "",
        "stderr": p.stderr or "",
    }


def load_yaml(path: Path):
    return yaml.safe_load(path.read_text(encoding="utf-8"))


def compiler_coroutine_preflight(cxx: str):
    """Check the exact compiler capability Verilator timing models need."""
    if not cxx:
        return {
            "status": "FAIL",
            "compiler": "",
            "reason": "No C++ compiler found.",
            "stdout": "",
            "stderr": "",
        }

    snippet = "int main(){return 0;}\n"
    try:
        p = subprocess.run(
            [cxx, "-std=c++20", "-fcoroutines", "-x", "c++", "-", "-fsyntax-only"],
            input=snippet,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except FileNotFoundError as e:
        return {
            "status": "FAIL",
            "compiler": cxx,
            "reason": str(e),
            "stdout": "",
            "stderr": "",
        }

    return {
        "status": "PASS" if p.returncode == 0 else "FAIL",
        "compiler": cxx,
        "reason": "" if p.returncode == 0 else "Compiler does not accept -std=c++20 -fcoroutines.",
        "stdout": p.stdout or "",
        "stderr": p.stderr or "",
    }


def load_discovery(path: Path):
    if not path.exists():
        return []
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def discovered_modules(rows, *, gap_id="", target_id="", project=""):
    out = []
    seen = set()
    for r in rows:
        if gap_id and r.get("gap_id") != gap_id:
            continue
        if target_id and r.get("target_id") != target_id:
            continue
        if project and r.get("source_project") != project:
            continue
        k = (r.get("source_project",""), r.get("module",""))
        if k not in seen:
            seen.add(k)
            out.append(r)
    return out


def parse_overall_report(path: Path, key_candidates):
    if not path.exists():
        return "MISSING", {}
    try:
        obj = json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return "PARSE_ERROR", {}
    for key in key_candidates:
        if key in obj:
            v = obj[key]
            if isinstance(v, dict) and "overall" in v:
                return str(v["overall"]), obj
            if isinstance(v, str):
                return v, obj
    return "UNKNOWN", obj


def read_candidate_gate(base: Path, project: str, module: str):
    p = base / "candidates" / project / module / "hard_gate_report.json"
    return parse_overall_report(p, ["hard_gate", "overall"])


def read_correctness(base: Path, project: str, module: str, profile: str):
    candidates = [
        base / "correctness_results" / project / module / profile / "correctness_report.json",
        base / "correctness_results" / project / module / "correctness_report.json",
    ]
    for p in candidates:
        if p.exists():
            return parse_overall_report(p, ["correctness_gate", "status", "overall"])
    return "MISSING", {}


def read_stateful(base: Path, project: str, profile: str):
    candidates = [
        base / "stateful_results" / project / "cc_rr_arb_tree" / profile / "stateful_correctness_report.json",
        base / "stateful_results" / project / "cc_rr_arb_tree" / profile / "correctness_report.json",
        base / "stateful_results" / project / "cc_rr_arb_tree" / "stateful_correctness_report.json",
    ]
    for p in candidates:
        if p.exists():
            return parse_overall_report(p, ["stateful_correctness_gate", "correctness_gate", "status", "overall"])
    return "MISSING", {}


def read_synthesis(base: Path, project: str, module: str, profile: str):
    p = base / "synthesis_results" / project / module / profile / "synthesis_report.json"
    return parse_overall_report(p, ["synthesis_gate", "status", "overall"])


def stage_record(status="NOT_RUN", supported=True, seconds=0.0, note="", command=None):
    return {
        "status": status,
        "supported": supported,
        "seconds": seconds,
        "note": note,
        "command": command or [],
    }


def candidate_overall(stages):
    # Unsupported optional stages do not fail a candidate. Any real FAIL does.
    vals = [v["status"] for v in stages.values() if v.get("supported", True)]
    if any(v in {"FAIL", "ERROR", "MISSING", "PARSE_ERROR"} for v in vals):
        return "FAIL"
    if any(v == "NOT_RUN" for v in vals):
        return "PARTIAL"
    return "PASS"


def html_report(report, path: Path):
    rows = []
    for c in report["candidates"]:
        s = c["stages"]
        rows.append(f"""
        <tr>
          <td>{html.escape(c['module'])}</td>
          <td>{html.escape(c['project'])}</td>
          <td>{html.escape(c.get('gap_id',''))}</td>
          <td>{html.escape(s['build']['status'])}</td>
          <td>{html.escape(s['correctness']['status'])}</td>
          <td>{html.escape(s['stateful']['status'])}</td>
          <td>{html.escape(s['synthesis']['status'])}</td>
          <td><b>{html.escape(c['overall'])}</b></td>
        </tr>""")
    doc = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>pyCircuit v0.6 Batch Pipeline Report</title>
<style>
body{{font-family:Arial,sans-serif;margin:32px;color:#1f2937}}
h1{{margin-bottom:4px}}
.meta{{color:#6b7280;margin-bottom:24px}}
table{{border-collapse:collapse;width:100%;font-size:14px}}
th,td{{border:1px solid #d1d5db;padding:9px;text-align:left}}
th{{background:#f3f4f6}}
pre{{background:#111827;color:#e5e7eb;padding:14px;border-radius:8px;overflow:auto}}
.pass{{color:#047857}}
</style>
</head>
<body>
<h1>pyCircuit Batch Mining & Benchmark v0.6.2</h1>
<div class="meta">
Target: {html.escape(report.get('selection',{}).get('description',''))}<br>
Profile: correctness={html.escape(report['profiles']['correctness'])},
stateful={html.escape(report['profiles']['stateful'])},
synthesis={html.escape(report['profiles']['synthesis'])}
</div>
<table>
<thead><tr>
<th>Module</th><th>Project</th><th>Gap ID</th>
<th>Build/Lint</th><th>Correctness</th><th>Stateful</th>
<th>Synthesis</th><th>Overall</th>
</tr></thead>
<tbody>
{''.join(rows)}
</tbody>
</table>
<h2>Pipeline Semantics</h2>
<pre>Discovery → Build/Dependency Closure → Compile/Lint Gate
          → Correctness → Stateful Correctness → Synthesis/QoR

UNSUPPORTED = candidate exists, but this design class does not yet
              have the corresponding benchmark adapter.
UNSUPPORTED is not treated as an RTL failure.</pre>
</body></html>"""
    path.write_text(doc, encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(
        description="pyCircuit v0.6 Batch Mining & Benchmark Orchestrator"
    )
    sel = ap.add_mutually_exclusive_group()
    sel.add_argument("--module", action="append", default=[], help="Exact module; repeatable")
    sel.add_argument("--gap-id", default="", help="Select discovered candidates by Gap Matrix ID")
    sel.add_argument("--target-id", default="", help="Select discovered candidates by target ID")
    sel.add_argument("--all-supported", action="store_true", help="Run all modules in benchmark_registry.yaml")
    ap.add_argument("--project", default="pulp_common_cells")
    ap.add_argument("--registry", default="benchmark_registry.yaml")
    ap.add_argument("--discovery", default="output/candidates_raw.csv")
    ap.add_argument("--discover", action="store_true", help="Run crawler before selection")
    ap.add_argument("--stages", default="build,correctness,stateful,synthesis",
                    help="Comma-separated subset of build,correctness,stateful,synthesis")
    ap.add_argument("--correctness-profile", choices=["smoke","standard","full"], default="smoke")
    ap.add_argument("--stateful-profile", choices=["smoke","standard","full"], default="smoke")
    ap.add_argument("--synthesis-profile", choices=["smoke","standard","scaling"], default="smoke")
    ap.add_argument("--frontend", choices=["auto","slang","native"], default="auto")
    ap.add_argument("--liberty", default="")
    ap.add_argument(
        "--cxx",
        default="",
        help="C++ compiler for Verilator --binary, e.g. /usr/bin/g++-11. "
             "Defaults to $CXX, otherwise g++.",
    )
    ap.add_argument("--output-root", default="pipeline_results")
    ap.add_argument("--keep-going", action="store_true", default=True)
    args = ap.parse_args()

    base = Path.cwd()
    registry = load_yaml(base / args.registry)["designs"]
    stages_req = [x.strip() for x in args.stages.split(",") if x.strip()]
    bad = [x for x in stages_req if x not in STAGES]
    if bad:
        raise SystemExit(f"Unknown stages: {bad}")

    # Verilator timing testbenches use C++20 coroutines. Check this once before
    # running candidate correctness/stateful stages, so an environment problem
    # is not misclassified as an RTL correctness failure.
    cxx = args.cxx or os.environ.get("CXX", "") or shutil.which("g++") or ""
    toolchain = compiler_coroutine_preflight(cxx)
    run_env = os.environ.copy()
    if cxx:
        run_env["CXX"] = cxx
        # Same GCC-family compiler for final link avoids ABI/tool mismatch.
        run_env["LINK"] = cxx

    needs_sim_compiler = any(s in stages_req for s in ("correctness", "stateful"))
    if needs_sim_compiler and toolchain["status"] != "PASS":
        print("=== Toolchain Preflight FAIL ===")
        print("CXX     :", cxx or "(not found)")
        print("reason  :", toolchain["reason"])
        if toolchain["stderr"]:
            print(toolchain["stderr"].strip())
        print()
        print("This is an ENVIRONMENT failure, not an RTL correctness failure.")
        print("Use GCC 10+ (prefer the installed g++-11/g++-12 if available), e.g.:")
        print("  python run_pipeline.py --all-supported --cxx /usr/bin/g++-11")
        raise SystemExit(2)

    if args.discover:
        r = run_cmd([sys.executable, "crawler.py", "--no-update"], base, env=run_env)
        if r["returncode"] != 0:
            print(r["stdout"])
            print(r["stderr"], file=sys.stderr)
            raise SystemExit("crawler failed")

    discovery_rows = load_discovery(base / args.discovery)

    selected = []
    selection_desc = ""
    if args.module:
        selection_desc = "module=" + ",".join(args.module)
        for m in args.module:
            selected.append({
                "module": m,
                "project": registry.get(m, {}).get("project", args.project),
                "gap_id": registry.get(m, {}).get("gap_id", ""),
                "target_id": registry.get(m, {}).get("target_id", ""),
                "source": "explicit",
            })
    elif args.gap_id or args.target_id:
        selection_desc = f"gap_id={args.gap_id or '*'}, target_id={args.target_id or '*'}"
        hits = discovered_modules(
            discovery_rows, gap_id=args.gap_id, target_id=args.target_id, project=args.project
        )
        for h in hits:
            selected.append({
                "module": h["module"],
                "project": h["source_project"],
                "gap_id": h.get("gap_id",""),
                "target_id": h.get("target_id",""),
                "source": "discovery",
                "discovery_match_score": h.get("discovery_match_score",""),
            })
    else:
        # Default is all currently benchmark-supported candidates.
        selection_desc = "all benchmark-supported candidates"
        for m, spec in registry.items():
            selected.append({
                "module": m,
                "project": spec.get("project", args.project),
                "gap_id": spec.get("gap_id",""),
                "target_id": spec.get("target_id",""),
                "source": "registry",
            })

    if not selected:
        raise SystemExit("No candidates selected.")

    out = base / args.output_root
    out.mkdir(parents=True, exist_ok=True)
    log_dir = out / "logs"
    log_dir.mkdir(exist_ok=True)

    report_candidates = []
    print("=== pyCircuit Batch Mining & Benchmark v0.6.2 ===")
    print("selection :", selection_desc)
    print("candidates:", len(selected))
    print("stages    :", ",".join(stages_req))
    print("CXX       :", cxx or "(not needed/not found)")
    if needs_sim_compiler:
        print("C++20 coro:", toolchain["status"])
    print()

    for idx, c in enumerate(selected, 1):
        module = c["module"]
        project = c["project"]
        spec = registry.get(module)
        print(f"[{idx}/{len(selected)}] {project}/{module}")

        entry = {
            **c,
            "benchmark_supported": bool(spec),
            "stages": {s: stage_record() for s in STAGES},
        }

        if spec is None:
            for s in STAGES:
                entry["stages"][s] = stage_record(
                    status="UNSUPPORTED", supported=False,
                    note="No benchmark adapter/registry entry yet."
                )
            entry["overall"] = "UNSUPPORTED"
            report_candidates.append(entry)
            print("  -> UNSUPPORTED: adapter not implemented")
            continue

        # BUILD / COMPILE HARD GATE
        if "build" in stages_req:
            cmd = [sys.executable, "build_candidate.py", module, "--project", project, "--lint"]
            rr = run_cmd(cmd, base, env=run_env)
            (log_dir / f"{project}__{module}__build.stdout.log").write_text(rr["stdout"], encoding="utf-8")
            (log_dir / f"{project}__{module}__build.stderr.log").write_text(rr["stderr"], encoding="utf-8")
            status, _ = read_candidate_gate(base, project, module)
            if rr["returncode"] != 0 and status not in PASSLIKE:
                status = "FAIL"
            entry["stages"]["build"] = stage_record(
                status=status, seconds=rr["seconds"], command=cmd
            )
        else:
            entry["stages"]["build"] = stage_record(status="NOT_RUN")

        build_ok = entry["stages"]["build"]["status"] in PASSLIKE or "build" not in stages_req

        # CORRECTNESS
        cspec = spec.get("correctness", {})
        if "correctness" not in stages_req:
            entry["stages"]["correctness"] = stage_record(status="NOT_RUN")
        elif not cspec.get("enabled", False):
            entry["stages"]["correctness"] = stage_record(
                status="UNSUPPORTED", supported=False, note="No correctness adapter."
            )
        elif not build_ok:
            entry["stages"]["correctness"] = stage_record(status="BLOCKED", note="Build gate failed.")
        else:
            cmd = [
                sys.executable, cspec.get("runner","run_correctness.py"),
                module, "--project", project, "--profile", args.correctness_profile
            ]
            if cxx:
                cmd += ["--cxx", cxx]
            rr = run_cmd(cmd, base, env=run_env)
            (log_dir / f"{project}__{module}__correctness.stdout.log").write_text(rr["stdout"], encoding="utf-8")
            (log_dir / f"{project}__{module}__correctness.stderr.log").write_text(rr["stderr"], encoding="utf-8")
            status, _ = read_correctness(base, project, module, args.correctness_profile)
            if rr["returncode"] == 0 and status in {"MISSING","UNKNOWN"}:
                status = "PASS"
            elif rr["returncode"] != 0:
                status = "FAIL"
            entry["stages"]["correctness"] = stage_record(
                status=status, seconds=rr["seconds"], command=cmd
            )

        # STATEFUL
        sspec = spec.get("stateful", {})
        if "stateful" not in stages_req:
            entry["stages"]["stateful"] = stage_record(status="NOT_RUN")
        elif not sspec.get("enabled", False):
            entry["stages"]["stateful"] = stage_record(
                status="UNSUPPORTED", supported=False,
                note="Design is combinational or no stateful adapter is required."
            )
        elif not build_ok:
            entry["stages"]["stateful"] = stage_record(status="BLOCKED", note="Build gate failed.")
        else:
            # Current v0.4.1 stateful harness is dedicated to cc_rr_arb_tree.
            cmd = [
                sys.executable, sspec.get("runner","run_stateful_correctness.py"),
                "--project", project, "--profile", args.stateful_profile
            ]
            if cxx:
                cmd += ["--cxx", cxx]
            rr = run_cmd(cmd, base, env=run_env)
            (log_dir / f"{project}__{module}__stateful.stdout.log").write_text(rr["stdout"], encoding="utf-8")
            (log_dir / f"{project}__{module}__stateful.stderr.log").write_text(rr["stderr"], encoding="utf-8")
            status, _ = read_stateful(base, project, args.stateful_profile)
            if rr["returncode"] == 0 and status in {"MISSING","UNKNOWN"}:
                status = "PASS"
            elif rr["returncode"] != 0:
                status = "FAIL"
            entry["stages"]["stateful"] = stage_record(
                status=status, seconds=rr["seconds"], command=cmd
            )

        # SYNTHESIS
        yspec = spec.get("synthesis", {})
        if "synthesis" not in stages_req:
            entry["stages"]["synthesis"] = stage_record(status="NOT_RUN")
        elif not yspec.get("enabled", False):
            entry["stages"]["synthesis"] = stage_record(
                status="UNSUPPORTED", supported=False, note="No synthesis adapter."
            )
        elif not build_ok:
            entry["stages"]["synthesis"] = stage_record(status="BLOCKED", note="Build gate failed.")
        else:
            cmd = [
                sys.executable, yspec.get("runner","run_synthesis.py"),
                module, "--project", project,
                "--profile", args.synthesis_profile,
                "--frontend", args.frontend,
            ]
            if args.liberty:
                cmd += ["--liberty", args.liberty]
            rr = run_cmd(cmd, base, env=run_env)
            (log_dir / f"{project}__{module}__synthesis.stdout.log").write_text(rr["stdout"], encoding="utf-8")
            (log_dir / f"{project}__{module}__synthesis.stderr.log").write_text(rr["stderr"], encoding="utf-8")
            status, _ = read_synthesis(base, project, module, args.synthesis_profile)
            if rr["returncode"] != 0 and status not in PASSLIKE:
                status = "FAIL"
            entry["stages"]["synthesis"] = stage_record(
                status=status, seconds=rr["seconds"], command=cmd
            )

        entry["overall"] = candidate_overall(entry["stages"])
        report_candidates.append(entry)
        print("  ->",
              "build", entry["stages"]["build"]["status"],
              "| corr", entry["stages"]["correctness"]["status"],
              "| state", entry["stages"]["stateful"]["status"],
              "| synth", entry["stages"]["synthesis"]["status"],
              "| overall", entry["overall"])

    overall = "PASS"
    real = [c for c in report_candidates if c["overall"] != "UNSUPPORTED"]
    if any(c["overall"] == "FAIL" for c in real):
        overall = "FAIL"
    elif any(c["overall"] == "PARTIAL" for c in real):
        overall = "PARTIAL"

    report = {
        "schema_version": "0.6.2",
        "pipeline_gate": overall,
        "selection": {
            "description": selection_desc,
            "candidate_count": len(selected),
        },
        "profiles": {
            "correctness": args.correctness_profile,
            "stateful": args.stateful_profile,
            "synthesis": args.synthesis_profile,
        },
        "toolchain": {
            "cxx": cxx,
            "coroutine_preflight": toolchain["status"],
            "coroutine_preflight_reason": toolchain["reason"],
        },
        "metric_semantics": {
            "UNSUPPORTED": "No benchmark adapter exists yet; this is not an RTL failure.",
            "BLOCKED": "A prerequisite gate failed, so this stage was not executed.",
            "PASS": "Requested stage completed successfully.",
            "PARTIAL": "Some requested/supported stage was not fully completed.",
        },
        "candidates": report_candidates,
    }

    json_path = out / "pipeline_report.json"
    json_path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    csv_path = out / "pipeline_summary.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        fields = [
            "project","module","gap_id","target_id","benchmark_supported",
            "build","correctness","stateful","synthesis","overall"
        ]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for c in report_candidates:
            w.writerow({
                "project": c["project"],
                "module": c["module"],
                "gap_id": c.get("gap_id",""),
                "target_id": c.get("target_id",""),
                "benchmark_supported": c["benchmark_supported"],
                "build": c["stages"]["build"]["status"],
                "correctness": c["stages"]["correctness"]["status"],
                "stateful": c["stages"]["stateful"]["status"],
                "synthesis": c["stages"]["synthesis"]["status"],
                "overall": c["overall"],
            })

    html_path = out / "pipeline_report.html"
    html_report(report, html_path)

    print()
    print("=== Batch Pipeline Gate ===")
    print("status :", overall)
    print("json   :", json_path)
    print("csv    :", csv_path)
    print("html   :", html_path)
    raise SystemExit(0 if overall in PASSLIKE else 1)


if __name__ == "__main__":
    main()
