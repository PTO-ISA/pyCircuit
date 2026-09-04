#!/usr/bin/env python3
"""Bounded, resumable validation of locally discovered RTL candidates.

The discovery crawler intentionally accepts broad keyword matches.  This
driver turns each unique ``(project, module, file)`` into a dependency closure,
Verilator lint gate, and generic Yosys synthesis/stat run.  Functional
simulation is reported as ADAPTER_REQUIRED unless a design-specific oracle is
available; no guessed testbench is used as evidence of correctness.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

from build_candidate import configured_parameter_overrides


PRIORITY = {"A": 0, "B": 1, "C": 2}


def safe(s: str) -> str:
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in s)


def run(cmd, cwd: Path, env: dict, timeout: int):
    start = time.time()
    try:
        p = subprocess.run(
            cmd,
            cwd=str(cwd),
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=False,
        )
        return {
            "status": "PASS" if p.returncode == 0 else "FAIL",
            "returncode": p.returncode,
            "seconds": round(time.time() - start, 3),
            "stdout": p.stdout or "",
            "stderr": p.stderr or "",
        }
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout or ""
        stderr = exc.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode("utf-8", errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        return {
            "status": "TIMEOUT",
            "returncode": None,
            "seconds": round(time.time() - start, 3),
            "stdout": stdout,
            "stderr": stderr + f"\nTimed out after {timeout}s",
        }
    except OSError as exc:
        return {
            "status": "ERROR",
            "returncode": None,
            "seconds": round(time.time() - start, 3),
            "stdout": "",
            "stderr": str(exc),
        }


def read_csv(path: Path):
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def dedupe_candidates(rows):
    unique = {}
    for row in rows:
        key = (row.get("source_project", ""), row.get("module", ""), row.get("file", ""))
        if not all(key):
            continue
        # ``candidates_frozen.csv`` is the canonical input produced by the
        # freeze step.  It aggregates all target matches, so its score and
        # primary target use explicit frozen-column names.  Keep accepting
        # the crawler's raw one-row-per-match format for compatibility with
        # older validation waves.
        if not row.get("discovery_match_score") and row.get("max_match_score"):
            row = dict(row)
            row["discovery_match_score"] = row.get("max_match_score", "0")
        if not row.get("target_id") and row.get("primary_target_id"):
            row = dict(row)
            row["target_id"] = row.get("primary_target_id", "")
        score = float(row.get("discovery_match_score") or 0)
        old = unique.get(key)
        if old is None or score > float(old.get("discovery_match_score") or 0):
            unique[key] = dict(row)
    out = list(unique.values())
    out.sort(
        key=lambda r: (
            PRIORITY.get((r.get("source_priority") or "").upper(), 99),
            -float(r.get("discovery_match_score") or 0),
            r.get("source_project", ""),
            r.get("module", ""),
            r.get("file", ""),
        )
    )
    return out


def infer_parameters(details_by_key, row):
    """Choose conservative elaboration values only for parameters with no default."""
    key = (row.get("source_project", ""), row.get("module", ""), row.get("file", ""))
    detail = details_by_key.get(key, {})
    try:
        params = json.loads(detail.get("parameters_json", "[]"))
    except (TypeError, ValueError):
        params = []
    out = {}
    for p in params:
        name = str(p.get("name", "")).strip()
        if not name or str(p.get("default", "")).strip():
            continue
        n = name.lower()
        if "width" in n or "bits" in n:
            value = 32
        elif "addr" in n:
            value = 8
        elif any(x in n for x in ("depth", "els", "num", "count", "inputs", "outputs", "ports", "ways", "lanes")):
            value = 4
        elif any(x in n for x in ("signed", "enable", "has_", "async")):
            value = 0
        else:
            value = 1
        out[name] = value
    return out


def candidate_id(row):
    text = "|".join(row.get(k, "") for k in ("source_project", "module", "file"))
    return f"{safe(row['module'])}__{hashlib.sha1(text.encode()).hexdigest()[:10]}"


def json_load(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {}


def parse_filelist(path: Path):
    includes, files = [], []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        if line.startswith("-I"):
            includes.append(line)
        else:
            files.append(line)
    return includes, files


def yosys_validate(candidate_dir: Path, top: str, liberty: str, timeout: int,
                   env: dict, parameters=None, defines=None, options=None):
    yosys = shutil.which("yosys", path=env.get("PATH", "")) or "/opt/oss-cad/oss-cad-suite/bin/yosys"
    if not Path(yosys).exists() and shutil.which(yosys) is None:
        return {"status": "TOOL_MISSING", "seconds": 0.0, "metrics": {}, "stdout": "", "stderr": "yosys not found"}
    filelist = candidate_dir / "candidate.f"
    if not filelist.exists():
        return {"status": "BLOCKED", "seconds": 0.0, "metrics": {}, "stdout": "", "stderr": "candidate.f missing"}
    includes, files = parse_filelist(filelist)
    if not files:
        return {"status": "BLOCKED", "seconds": 0.0, "metrics": {}, "stdout": "", "stderr": "empty filelist"}
    # candidate.f paths are already relative to candidate_dir.  Yosys has no
    # Verilator-style -F mode, so expand all include options and source files.
    include_text = (" " + " ".join(includes)) if includes else ""
    define_text = ""
    for define in (defines or []):
        value = str(define).strip()
        if not value:
            continue
        if value.startswith("-D"):
            value = value[2:]
        # Yosys' read_verilog accepts the same -DNAME[=VALUE] form as
        # Verilator.  Keep project build-context defines in the manifest and
        # pass them to both frontends for reproducible elaboration.
        # This is a Yosys command string, not a shell command: shell-style
        # quoting would become part of the macro name (notably for Verilog
        # literals such as ``1'b0``).
        define_text += " -D" + value
    options = options or {}
    requested_frontend = str(options.get("frontend", "auto")).lower()
    skip_abc = bool(options.get("skip_abc", False))
    allow_use_before_declare = bool(options.get("allow_use_before_declare", False))
    max_generate_steps = options.get("max_generate_steps")
    # Recent OSS CAD Suite builds ship Yosys' slang SystemVerilog frontend.
    # Prefer it for typed structs/interfaces (the classic read_verilog
    # frontend rejects constructs such as ``shortint`` and parameter types),
    # while retaining the classic frontend as a portable fallback.
    try:
        probe = subprocess.run([yosys, "-Q", "-p", "help read_slang"],
                               env=env, text=True, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, check=False, timeout=10)
        slang = probe.returncode == 0 and "read_slang" in (probe.stdout or "")
    except (OSError, subprocess.TimeoutExpired):
        slang = False
    source_text = " ".join(shlex.quote(f) for f in files)
    # The slang frontend elaborates parameterized modules while reading and
    # therefore does not leave a parametric RTLIL module for a later
    # ``chparam`` pass.  Feed top-level overrides through slang's native -G
    # option; the classic frontend keeps the historical chparam path below.
    slang_param_text = "".join(
        f" -G{name}={value}" for name, value in (parameters or {}).items()
    )

    def make_script(frontend_name: str) -> str:
        if frontend_name == "slang":
            read = (
                "read_slang --std 1800-2017 --top " + shlex.quote(top)
                + slang_param_text + define_text + include_text + " " + source_text
            )
            if allow_use_before_declare:
                read = read.replace("read_slang ", "read_slang --allow-use-before-declare ", 1)
            if max_generate_steps:
                read = read.replace("read_slang ", f"read_slang --max-generate-steps {int(max_generate_steps)} ", 1)
        else:
            read = "read_verilog -sv" + define_text + include_text + " " + source_text
        # Apply explicit/inferred top parameters immediately after reading the
        # design.  Some reusable RTL declares parameters without defaults
        # (for example ``NumInputs`` in clock trees); running hierarchy
        # validation first makes Yosys reject the module before ``chparam``
        # gets a chance to specialize it.
        script_text = read
        if frontend_name != "slang":
            for name, value in (parameters or {}).items():
                script_text += f"; chparam -set {name} {value} {top}"
        script_text += f"; hierarchy -check -top {top}"
        script_text += "; proc; opt; flatten; opt_clean"
        if liberty and not skip_abc:
            script_text += f"; abc -liberty {shlex.quote(liberty)}"
        return script_text + "; tee -o stat.json stat -json -top " + top

    if requested_frontend == "slang":
        frontend = "slang" if slang else "verilog"
    elif requested_frontend in {"verilog", "classic"}:
        frontend = "verilog"
    else:
        frontend = "slang" if slang else "verilog"
    result = run([yosys, "-Q", "-p", make_script(frontend)], candidate_dir, env, timeout)

    # BaseJump STL uses BSG_ABSTRACT_MODULE() at the end of many source
    # files.  The slang frontend intentionally promotes that abstract unit
    # to the top and then rejects the concrete DUT name, while Yosys' classic
    # Verilog frontend keeps the concrete module as top.  Retry only for this
    # characteristic elaboration failure; all other slang errors remain
    # visible and are not silently converted into success.
    diagnostic = (result.get("stdout", "") + "\n" + result.get("stderr", "")).lower()
    if (
        frontend == "slang"
        and result.get("status") != "PASS"
        and "not a valid top-level module" in diagnostic
    ):
        stat_path = candidate_dir / "stat.json"
        if stat_path.exists():
            stat_path.unlink()
        fallback = run([yosys, "-Q", "-p", make_script("verilog")], candidate_dir, env, timeout)
        fallback["fallback_reason"] = "slang abstract-top mismatch"
        fallback["slang_result"] = {
            "status": result.get("status"),
            "returncode": result.get("returncode"),
            "seconds": result.get("seconds"),
        }
        result = fallback
        frontend = "verilog-fallback"
    stat_path = candidate_dir / "stat.json"
    stats = json_load(stat_path) if stat_path.exists() else {}
    module_stats = stats.get("modules", {}).get("\\" + top, stats.get("modules", {}).get(top, {}))
    if not module_stats and stats.get("modules"):
        module_stats = next(iter(stats["modules"].values()))
    metrics = {
        "num_cells": module_stats.get("num_cells"),
        "num_wire_bits": module_stats.get("num_wire_bits"),
        "num_processes": module_stats.get("num_processes"),
        "num_cells_by_type": module_stats.get("num_cells_by_type", {}),
        "area": module_stats.get("area"),
        "ppa_mode": "liberty" if liberty else "structural_proxy",
    }
    result.update({"tool": yosys, "frontend": frontend, "metrics": metrics,
                   "stat_file": str(stat_path), "options": options})
    return result


def main():
    ap = argparse.ArgumentParser(description="pyCircuit v0.4 bounded candidate validator")
    ap.add_argument("--discovery", default="build/discovery-v0.4/all-local-v0.4/candidates_raw.csv")
    ap.add_argument("--details", default="", help="candidate_details.csv; defaults to the discovery directory sibling")
    ap.add_argument("--sources", default="sources.expanded.v0.3.json")
    ap.add_argument("--workdir", default="build/source-cache-v0.4")
    ap.add_argument("--candidate-root", default="build/candidate-validation-v0.4")
    ap.add_argument("--output-root", default="build/batch-validation-v0.4")
    ap.add_argument("--offset", type=int, default=0)
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--timeout-sec", type=int, default=120)
    ap.add_argument("--yosys-timeout-sec", type=int, default=120)
    ap.add_argument("--liberty", default="")
    ap.add_argument("--batch-name", default="priority-batch")
    ap.add_argument("--resume", action="store_true")
    args = ap.parse_args()

    base = Path.cwd()
    discovery = base / args.discovery
    candidates = dedupe_candidates(read_csv(discovery))
    sources_path = Path(args.sources)
    if not sources_path.is_absolute():
        sources_path = (base / sources_path).resolve()
    source_cfg = json_load(sources_path)
    source_by_project = {
        str(s.get("project")): s for s in source_cfg.get("sources", [])
        if isinstance(s, dict) and s.get("project")
    }
    details_path = Path(args.details) if args.details else discovery.parent / "candidate_details.csv"
    detail_rows = read_csv(base / details_path)
    details_by_key = {
        (r.get("source_project", ""), r.get("module", ""), r.get("file", "")): r
        for r in detail_rows
    }
    selected = candidates[args.offset : args.offset + args.limit]
    out = base / args.output_root / safe(args.batch_name)
    out.mkdir(parents=True, exist_ok=True)
    logs = out / "logs"
    logs.mkdir(exist_ok=True)
    results = []

    env = os.environ.copy()
    env["PATH"] = "/opt/oss-cad/oss-cad-suite/bin:/usr/bin:" + env.get("PATH", "")
    python = sys.executable
    print(f"=== candidate batch {args.batch_name} ===")
    print(f"range: offset={args.offset}, limit={args.limit}, selected={len(selected)}/{len(candidates)}")
    print(f"stages: closure -> verilator lint -> generic yosys -> structural PPA proxy")

    for index, row in enumerate(selected, args.offset + 1):
        cid = candidate_id(row)
        project, module, top_file = row["source_project"], row["module"], row["file"]
        result_path = out / f"{safe(project)}__{cid}.json"
        if args.resume and result_path.exists():
            results.append(json_load(result_path))
            print(f"[{index}/{len(candidates)}] {project}/{module} -> RESUMED")
            continue
        print(f"[{index}/{len(candidates)}] {project}/{module}")
        candidate_dir = (base / args.candidate_root / project / cid).resolve()
        source = source_by_project.get(project, {})
        parameters = infer_parameters(details_by_key, row)
        # Fixed source-level values are authoritative for structural
        # validation; inferred values only fill parameters that have no
        # default and are not explicitly configured.
        fixed_parameters = configured_parameter_overrides(source, module)
        parameters.update(fixed_parameters)
        raw_yosys_overrides = source.get("yosys_overrides", {}) or {}
        yosys_options = {}
        if isinstance(raw_yosys_overrides, dict):
            wildcard_yosys = raw_yosys_overrides.get("*", {})
            if isinstance(wildcard_yosys, dict):
                yosys_options.update(wildcard_yosys)
            exact_yosys = raw_yosys_overrides.get(module, {})
            if isinstance(exact_yosys, dict):
                yosys_options.update(exact_yosys)
        yosys_timeout = int(yosys_options.get("timeout_sec", args.yosys_timeout_sec))
        build_cmd = [
            python,
            "build_candidate.py",
            module,
            "--project",
            project,
            "--sources",
            str(sources_path),
            "--workdir",
            args.workdir,
            "--candidate-root",
            args.candidate_root,
            "--candidate-id",
            cid,
            "--top-file",
            top_file,
            "--lint",
        ]
        for name, value in parameters.items():
            build_cmd += ["--param", f"{name}={value}"]
        build = run(build_cmd, base, env, args.timeout_sec)
        (logs / f"{safe(project)}__{cid}__build.stdout.log").write_text(build["stdout"], encoding="utf-8")
        (logs / f"{safe(project)}__{cid}__build.stderr.log").write_text(build["stderr"], encoding="utf-8")
        manifest = json_load(candidate_dir / "manifest.json")
        gate = json_load(candidate_dir / "hard_gate_report.json")
        closure = manifest.get("closure_status", "MISSING")
        verilator_status = gate.get("hard_gate", {}).get("verilator_compile", "MISSING")
        closure_status = "PASS" if closure == "COMPLETE" else ("FAIL" if closure else "MISSING")
        if build["status"] != "PASS" and not manifest:
            closure_status = "ERROR"

        # A partial closure can still be useful for structural synthesis when
        # the unresolved item is an external assertion/UVM/helper module and
        # Verilator has already elaborated the selected top.  Keep the closure
        # status visible, but do not suppress Yosys in that case.
        validation_top = manifest.get("validation_top", module)
        defines = manifest.get("defines", [])
        validation_parameters = parameters if validation_top == module else {}
        if verilator_status == "PASS" and manifest:
            yosys_result = yosys_validate(candidate_dir, validation_top, args.liberty, yosys_timeout, env, validation_parameters, defines, yosys_options)
        else:
            yosys_result = {"status": "BLOCKED", "seconds": 0.0, "metrics": {}, "stdout": "", "stderr": "closure or Verilator gate failed"}
        (logs / f"{safe(project)}__{cid}__yosys.stdout.log").write_text(yosys_result.get("stdout", ""), encoding="utf-8")
        (logs / f"{safe(project)}__{cid}__yosys.stderr.log").write_text(yosys_result.get("stderr", ""), encoding="utf-8")

        # A generic harness cannot infer the semantic oracle for arbitrary RTL
        # interfaces.  Keep this explicit so it cannot be mistaken for PASS.
        simulation = {
            "status": "ADAPTER_REQUIRED",
            "supported": False,
            "note": "No design-specific functional oracle/testbench registered.",
        }
        if verilator_status == "PASS" and yosys_result["status"] == "PASS":
            overall = "PARTIAL"
        else:
            overall = "FAIL"
        report = {
            "schema": "pycircuit-rtl-candidate-validation-v0.4",
            "candidate": {
                "candidate_id": cid,
                "source_project": project,
                "module": module,
                "top_file": top_file,
                "validation_top": validation_top,
                "target_id": row.get("target_id", ""),
                "gap_id": row.get("gap_id", ""),
                "priority": row.get("source_priority", ""),
                "discovery_match_score": row.get("discovery_match_score", ""),
                "parameter_overrides": parameters,
            },
            "stages": {
                "dependency_closure": {"status": closure_status, "manifest": str(candidate_dir / "manifest.json")},
                "verilator_lint": {"status": verilator_status, "report": str(candidate_dir / "lint_report.json")},
                "simulation": simulation,
                "yosys_synthesis": {k: v for k, v in yosys_result.items() if k not in {"stdout", "stderr"}},
                "ppa": {
                    "status": "MEASURED" if yosys_result.get("status") == "PASS" else "NOT_MEASURED",
                    "mode": yosys_result.get("metrics", {}).get("ppa_mode", "none"),
                    "metrics": yosys_result.get("metrics", {}),
                },
            },
            "overall": overall,
            "toolchain": {"verilator": shutil.which("verilator", path=env["PATH"]) or "", "yosys": shutil.which("yosys", path=env["PATH"]) or ""},
        }
        result_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        results.append(report)
        print(
            f"  -> closure={closure_status} verilator={verilator_status} "
            f"yosys={yosys_result['status']} simulation=ADAPTER_REQUIRED overall={overall}"
        )

    summary = {
        "schema": "pycircuit-rtl-candidate-batch-v0.4",
        "batch_name": args.batch_name,
        "discovery": str(discovery),
        "offset": args.offset,
        "limit": args.limit,
        "total_discovered_candidates": len(candidates),
        "selected_count": len(selected),
        "counts": {
            "overall_partial": sum(r.get("overall") == "PARTIAL" for r in results),
            "overall_fail": sum(r.get("overall") == "FAIL" for r in results),
            "closure_pass": sum(r.get("stages", {}).get("dependency_closure", {}).get("status") == "PASS" for r in results),
            "verilator_pass": sum(r.get("stages", {}).get("verilator_lint", {}).get("status") == "PASS" for r in results),
            "yosys_pass": sum(r.get("stages", {}).get("yosys_synthesis", {}).get("status") == "PASS" for r in results),
            "simulation_adapter_required": sum(r.get("stages", {}).get("simulation", {}).get("status") == "ADAPTER_REQUIRED" for r in results),
        },
        "results": [str(out / f"{safe(r['candidate']['source_project'])}__{r['candidate']['candidate_id']}.json") for r in results],
    }
    (out / "batch_report.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    with (out / "batch_summary.csv").open("w", newline="", encoding="utf-8") as f:
        fields = ["candidate_id", "source_project", "module", "target_id", "closure", "verilator", "simulation", "yosys", "ppa", "overall"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in results:
            c, s = r["candidate"], r["stages"]
            w.writerow({
                "candidate_id": c["candidate_id"], "source_project": c["source_project"], "module": c["module"], "target_id": c["target_id"],
                "closure": s["dependency_closure"]["status"], "verilator": s["verilator_lint"]["status"], "simulation": s["simulation"]["status"],
                "yosys": s["yosys_synthesis"]["status"], "ppa": s["ppa"]["status"], "overall": r["overall"],
            })
    print(f"wrote {out / 'batch_report.json'}")
    print(json.dumps(summary["counts"], ensure_ascii=False))


if __name__ == "__main__":
    main()
