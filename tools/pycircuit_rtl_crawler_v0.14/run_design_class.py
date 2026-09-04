#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import html
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

import yaml

from design_class_adapters import generate_adapter
from design_class_tb import generate_rr_property_tb, generate_fifo_property_tb, generate_popcount_property_tb
from run_synthesis import (
    parse_candidate_filelist,
    yosys_has_read_slang,
    yosys_version,
    write_generic_script,
    extract_json,
    flatten_metrics,
    parse_ltp,
    write_liberty_script,
    find_area,
)

def run(cmd, cwd=None, env=None):
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

def safe(s):
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in s)

def read_closure_status(path: Path):
    if not path.exists():
        return ""
    try:
        obj = json.loads(path.read_text(encoding="utf-8"))
        return obj.get("closure_status", "")
    except Exception:
        return ""


def configured_lint(candidate_dir: Path, wrapper: Path, case_dir: Path, defines=None):
    """Compile/lint the fully parameterized canonical wrapper.

    This is intentionally configuration-specific. Some libraries, notably
    BaseJump STL, use mandatory parameters without defaults. Linting the native
    module as a top before a benchmark configuration is applied can therefore
    produce a false build failure.
    """
    verilator = shutil.which("verilator")
    if not verilator:
        return {"status": "TOOL_MISSING", "returncode": None, "command": []}

    cmd = [
        verilator,
        "--lint-only",
        "--Wno-fatal",
        "--top-module", "pyc_synth_top",
    ]
    for define in (defines or []):
        cmd += [f"-D{define}"]
    cmd += [
        "-F", str((candidate_dir / "candidate.f").resolve()),
        str(wrapper.resolve()),
    ]
    p = run(cmd, cwd=case_dir)
    (case_dir / "configured_lint_stdout.log").write_text(
        p.stdout or "", encoding="utf-8"
    )
    (case_dir / "configured_lint_stderr.log").write_text(
        p.stderr or "", encoding="utf-8"
    )
    return {
        "status": "PASS" if p.returncode == 0 else "FAIL",
        "returncode": p.returncode,
        "command": cmd,
    }

def choose_frontend(requested, yosys):
    avail = yosys_has_read_slang(yosys)
    if requested == "auto":
        return ("slang" if avail else "native"), avail
    if requested == "slang" and not avail:
        raise SystemExit("--frontend slang requested but read_slang is unavailable.")
    return requested, avail

def property_correctness(base, project, module, candidate_dir, wrapper, tb, cxx, defines=None):
    verilator = shutil.which("verilator")
    if not verilator:
        return {"status":"TOOL_MISSING","returncode":None}
    obj = tb.parent / "obj_dir"
    cmd = [
        verilator, "--binary", "--timing", "--Wno-fatal",
        "--top-module", "tb",
        "--Mdir", str(obj),
    ]
    if cxx:
        cmd += [
            "-MAKEFLAGS", f"CXX={cxx}",
            "-MAKEFLAGS", f"LINK={cxx}",
        ]
    for define in (defines or []):
        cmd += [f"-D{define}"]
    cmd += [
        "-F", str((candidate_dir / "candidate.f").resolve()),
        str(wrapper.resolve()),
        str(tb.resolve()),
    ]
    cp = run(cmd, cwd=tb.parent)
    (tb.parent / "verilator_compile_stdout.log").write_text(cp.stdout or "", encoding="utf-8")
    (tb.parent / "verilator_compile_stderr.log").write_text(cp.stderr or "", encoding="utf-8")
    if cp.returncode != 0:
        return {"status":"COMPILE_FAIL","returncode":cp.returncode,"command":cmd}

    exe = obj / "Vtb"
    if not exe.exists():
        return {"status":"COMPILE_FAIL","returncode":cp.returncode,"command":cmd,"note":"Vtb missing"}
    sp = run([str(exe)], cwd=tb.parent)
    (tb.parent / "sim_stdout.log").write_text(sp.stdout or "", encoding="utf-8")
    (tb.parent / "sim_stderr.log").write_text(sp.stderr or "", encoding="utf-8")
    status = "PASS" if sp.returncode == 0 and "PYC_DC_PASS" in (sp.stdout or "") else "FAIL"
    return {
        "status":status,
        "returncode":sp.returncode,
        "compile_returncode":cp.returncode,
        "command":cmd,
    }

def synth_case(candidate_dir, wrapper, case_dir, frontend, liberty, defines=None):
    yosys = shutil.which("yosys")
    if not yosys:
        return {"status":"TOOL_MISSING","cells":None,"depth":None,"area":None}

    srcs, incs = parse_candidate_filelist(candidate_dir / "candidate.f")
    ys = write_generic_script(srcs, incs, wrapper, case_dir, frontend, defines)
    p = run([yosys, "-s", str(ys)], cwd=case_dir)
    (case_dir / "yosys_stdout.log").write_text(p.stdout or "", encoding="utf-8")
    (case_dir / "yosys_stderr.log").write_text(p.stderr or "", encoding="utf-8")
    if p.returncode != 0:
        return {"status":"FAIL","cells":None,"depth":None,"area":None,"returncode":p.returncode}

    stat = extract_json(case_dir / "generic_stat.json")
    m = flatten_metrics(stat)
    depth = parse_ltp(case_dir / "ltp.log")
    area = None
    lib_status = "NOT_REQUESTED"
    if liberty:
        lys = write_liberty_script(srcs, incs, wrapper, case_dir, liberty, frontend, defines)
        lp = run([yosys, "-s", str(lys)], cwd=case_dir)
        (case_dir / "yosys_liberty_stdout.log").write_text(lp.stdout or "", encoding="utf-8")
        (case_dir / "yosys_liberty_stderr.log").write_text(lp.stderr or "", encoding="utf-8")
        lib_status = "PASS" if lp.returncode == 0 else "FAIL"
        if lp.returncode == 0:
            area = find_area(extract_json(case_dir / "liberty_stat.json"))

    return {
        "status":"PASS",
        "cells":m.get("num_cells"),
        "depth":depth,
        "area":area,
        "liberty_status":lib_status,
    }

def pareto_flags(rows):
    usable = [r for r in rows if r.get("correctness") == "PASS"
              and r.get("synthesis") == "PASS"
              and isinstance(r.get("cells"), int)
              and isinstance(r.get("depth"), int)]
    for r in rows:
        r["pareto"] = False
    for a in usable:
        dominated = False
        for b in usable:
            if a is b:
                continue
            if (b["cells"] <= a["cells"] and b["depth"] <= a["depth"]
                and (b["cells"] < a["cells"] or b["depth"] < a["depth"])):
                dominated = True
                break
        a["pareto"] = not dominated

def write_html(report, path):
    rows = []
    for r in report["rows"]:
        rows.append(
            "<tr>"
            f"<td>{html.escape(r['project'])}</td>"
            f"<td>{html.escape(r['module'])}</td>"
            f"<td>{html.escape(r['config'])}</td>"
            f"<td>{html.escape(r['closure'])}</td>"
            f"<td>{html.escape(r['build'])}</td>"
            f"<td>{html.escape(r['correctness'])}</td>"
            f"<td>{html.escape(r['synthesis'])}</td>"
            f"<td>{r.get('cells','')}</td>"
            f"<td>{r.get('depth','')}</td>"
            f"<td>{r.get('area','')}</td>"
            f"<td>{'YES' if r.get('pareto') else ''}</td>"
            "</tr>"
        )
    doc = f"""<!doctype html>
<html><head><meta charset="utf-8">
<title>pyCircuit Design-Class Benchmark</title>
<style>
body{{font-family:Arial,sans-serif;margin:32px;color:#1f2937}}
table{{border-collapse:collapse;width:100%}}
th,td{{border:1px solid #d1d5db;padding:8px;text-align:left}}
th{{background:#f3f4f6}}
pre{{background:#111827;color:#e5e7eb;padding:14px;border-radius:8px}}
</style></head><body>
<h1>pyCircuit Design-Class Benchmark</h1>
<p>Canonical property benchmark + same-flow synthesis comparison across independent open-source implementations.</p>
<pre>Native RTL
   ↓
Design-Class Adapter
   ↓
clk/rst/req/accept → valid + one-hot selection
   ↓
Common Property Test
   ↓
Same Slang/Yosys Flow
   ↓
Cells / Logic Depth / optional Liberty Area
   ↓
Pareto Frontier</pre>
<table><thead><tr>
<th>Project</th><th>Module</th><th>Config</th><th>Closure</th><th>Configured Build</th>
<th>Correctness</th><th>Synthesis</th><th>Cells</th><th>Depth</th>
<th>Area</th><th>Pareto</th>
</tr></thead><tbody>{''.join(rows)}</tbody></table>
</body></html>"""
    path.write_text(doc, encoding="utf-8")

def main():
    ap = argparse.ArgumentParser(description="pyCircuit v0.7 Design-Class Benchmark Runner")
    ap.add_argument("--class-id", default="DF-09")
    ap.add_argument("--profile", choices=["smoke","standard","scaling"], default="smoke")
    ap.add_argument("--specs", default="design_class_specs.yaml")
    ap.add_argument("--sources", default="sources.json")
    ap.add_argument("--workdir", default="work")
    ap.add_argument("--candidate-root", default="candidates")
    ap.add_argument("--output-root", default="design_class_results")
    ap.add_argument("--frontend", choices=["auto","slang","native"], default="auto")
    ap.add_argument("--cxx", default="")
    ap.add_argument("--liberty", default="")
    ap.add_argument("--skip-correctness", action="store_true")
    ap.add_argument("--skip-synthesis", action="store_true")
    ap.add_argument("--update", action="store_true")
    args = ap.parse_args()

    base = Path.cwd()
    specs = yaml.safe_load((base / args.specs).read_text(encoding="utf-8"))
    dc = specs["design_classes"].get(args.class_id)
    if not dc:
        raise SystemExit(f"Unknown design class {args.class_id}")
    configs = dc["profiles"][args.profile]
    candidates = dc["candidates"]

    yosys = shutil.which("yosys")
    frontend = ""
    slang_available = False
    if not args.skip_synthesis:
        if not yosys:
            raise SystemExit("Yosys not found.")
        frontend, slang_available = choose_frontend(args.frontend, yosys)

    liberty = Path(args.liberty).expanduser().resolve() if args.liberty else None
    if liberty and not liberty.exists():
        raise SystemExit(f"Liberty not found: {liberty}")

    outroot = base / args.output_root / args.class_id / args.profile
    outroot.mkdir(parents=True, exist_ok=True)
    rows = []

    print("=== pyCircuit Design-Class Benchmark v0.14 ===")
    print("class      :", args.class_id, dc["operation"])
    print("profile    :", args.profile)
    print("candidates :", len(candidates))
    print("configs    :", len(configs))
    if not args.skip_synthesis:
        print("yosys      :", yosys_version())
        print("frontend   :", frontend)
    print("CXX        :", args.cxx or "(Verilator default)")
    print()

    for cand in candidates:
        project = cand["project"]
        module = cand["module"]
        print(f"[candidate] {project}/{module}")

        # Stage A: repository-level dependency closure only.
        # Do NOT lint the raw native module as top here; mandatory parameters
        # may have no default and are only meaningful after config adaptation.
        build_cmd = [
            sys.executable, "build_candidate.py", module,
            "--project", project,
            "--sources", args.sources,
            "--workdir", args.workdir,
            "--candidate-root", args.candidate_root,
        ]
        if cand.get("source_hint"):
            build_cmd += ["--top-file", cand["source_hint"]]
        for prune_mod in cand.get("prune_modules", []):
            build_cmd += ["--prune-module", prune_mod]
        for prune_pkg in cand.get("prune_packages", []):
            build_cmd += ["--prune-package", prune_pkg]
        if args.update:
            build_cmd.append("--update")

        bp = run(build_cmd, cwd=base)
        cdir = base / args.candidate_root / project / module
        closure_status = read_closure_status(cdir / "manifest.json")
        if bp.returncode != 0 or closure_status != "COMPLETE":
            closure_gate = "FAIL"
        else:
            closure_gate = "PASS"

        for cfg in configs:
            case = outroot / safe(project) / safe(module) / cfg["name"]
            if case.exists():
                shutil.rmtree(case)
            case.mkdir(parents=True)

            wrapper = case / "adapter_top.sv"
            wrapper.write_text(
                generate_adapter(cand["adapter"], cfg),
                encoding="utf-8"
            )

            # Stage B: configuration-specific build/lint using canonical wrapper
            # as the top. This is the actual comparable Build Gate.
            configured_build = "SKIPPED"
            configured_lint_report = {}
            cand_defines = cand.get("defines", [])
            if closure_gate == "PASS":
                configured_lint_report = configured_lint(
                    cdir, wrapper, case, cand_defines
                )
                configured_build = configured_lint_report["status"]
                (case / "configured_build_report.json").write_text(
                    json.dumps(configured_lint_report, indent=2) + "\n",
                    encoding="utf-8",
                )
            else:
                configured_build = "BLOCKED"

            corr_status = "SKIPPED"
            if not args.skip_correctness and configured_build == "PASS":
                tb = case / "tb.sv"
                kind = dc.get("benchmark_kind", "round_robin")
                if kind == "fifo_sync":
                    tb_text = generate_fifo_property_tb(
                        int(cfg["data_width"]),
                        int(cfg["capacity"]),
                        int(cfg.get("random_steps", 160)),
                    )
                elif kind == "popcount":
                    tb_text = generate_popcount_property_tb(
                        int(cfg["width"]),
                        int(cfg.get("random_tests", 256)),
                    )
                else:
                    tb_text = generate_rr_property_tb(
                        int(cfg["n"]),
                        int(cfg.get("accepted_rounds", 2)),
                    )
                tb.write_text(tb_text, encoding="utf-8")
                cr = property_correctness(base, project, module, cdir, wrapper, tb, args.cxx, cand_defines)
                corr_status = cr["status"]
                (case / "correctness_case_report.json").write_text(
                    json.dumps(cr, indent=2) + "\n", encoding="utf-8"
                )

            synth_status = "SKIPPED"
            cells = depth = area = None
            if not args.skip_synthesis and configured_build == "PASS":
                sr = synth_case(cdir, wrapper, case, frontend, liberty, cand_defines)
                synth_status = sr["status"]
                cells, depth, area = sr.get("cells"), sr.get("depth"), sr.get("area")
                (case / "synthesis_case_report.json").write_text(
                    json.dumps(sr, indent=2) + "\n", encoding="utf-8"
                )

            row = {
                "project":project,
                "module":module,
                "adapter":cand["adapter"],
                "config":cfg["name"],
                "n":cfg.get("n", ""),
                "data_width":cfg.get("data_width", ""),
                "capacity":cfg.get("capacity", ""),
                "width":cfg.get("width", ""),
                "closure":closure_gate,
                "build":configured_build,
                "correctness":corr_status,
                "synthesis":synth_status,
                "cells":cells,
                "depth":depth,
                "area":area,
                "pareto":False,
            }
            rows.append(row)
            print("  ", cfg["name"],
                  "| closure", closure_gate,
                  "| build", configured_build,
                  "| corr", corr_status,
                  "| synth", synth_status,
                  "| cells", cells,
                  "| depth", depth)

    # Pareto is evaluated per configuration, never across different N.
    for cfg in configs:
        subset = [r for r in rows if r["config"] == cfg["name"]]
        pareto_flags(subset)

    report = {
        "schema_version":"0.14",
        "design_class":args.class_id,
        "operation":dc["operation"],
        "profile":args.profile,
        "frontend":frontend,
        "yosys_version":yosys_version() if yosys else "",
        "liberty":str(liberty) if liberty else "",
        "canonical_contract":dc["canonical_contract"],
        "rows":rows,
        "ranking_semantics":{
            "pareto":"A candidate is Pareto-optimal if no other passing implementation has both <= cells and <= logic depth with at least one strict improvement.",
            "score":"No arbitrary weighted score is assigned in v0.7."
        }
    }

    (outroot / "comparison_report.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    with (outroot / "comparison.csv").open("w", newline="", encoding="utf-8") as f:
        fields = ["project","module","adapter","config","n","data_width","capacity","width","closure","build","correctness","synthesis","cells","depth","area","pareto"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    write_html(report, outroot / "comparison.html")

    real_fail = any(
        r["closure"] == "FAIL"
        or r["build"] in {"FAIL", "BLOCKED", "TOOL_MISSING"}
        or (not args.skip_correctness and r["correctness"] not in {"PASS","SKIPPED"})
        or (not args.skip_synthesis and r["synthesis"] not in {"PASS","SKIPPED"})
        for r in rows
    )

    print()
    print("report :", outroot / "comparison_report.json")
    print("csv    :", outroot / "comparison.csv")
    print("html   :", outroot / "comparison.html")
    raise SystemExit(1 if real_fail else 0)

if __name__ == "__main__":
    main()
