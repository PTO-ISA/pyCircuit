#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import time
from pathlib import Path

import yaml

from stateful_tb_generator import generate_stateful_tb


RESULT_RE = re.compile(
    r"PYC_STATEFUL_RESULT\s+(PASS|FAIL)\s+module=cc_rr_arb_tree\s+"
    r"mode=([A-Za-z0-9_]+)\s+tests=(\d+)\s+errors=(\d+)"
)


def run(cmd, cwd=None):
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def case_id(config, seed):
    payload = json.dumps({"config": config, "seed": seed}, sort_keys=True)
    h = hashlib.sha1(payload.encode()).hexdigest()[:10]
    return f"{config['name']}__{h}"


def main():
    ap = argparse.ArgumentParser(description="pyCircuit Stateful Correctness v0.4.1")
    ap.add_argument("--profile", choices=["smoke", "standard", "full"], default="standard")
    ap.add_argument("--project", default="pulp_common_cells")
    ap.add_argument("--seed", type=lambda x: int(x, 0), default=0x41A5EED)
    ap.add_argument("--specs", default="stateful_specs.yaml")
    ap.add_argument("--candidate-root", default="candidates")
    ap.add_argument("--output-root", default="stateful_results")
    ap.add_argument("--keep-obj", action="store_true")
    ap.add_argument(
        "--cxx",
        default="",
        help="C++ compiler used by Verilator internal make, e.g. /usr/bin/g++-10",
    )
    args = ap.parse_args()

    base = Path.cwd()
    specs = yaml.safe_load((base / args.specs).read_text(encoding="utf-8"))
    configs = specs["profiles"][args.profile]

    candidate_dir = base / args.candidate_root / args.project / "cc_rr_arb_tree"
    filelist = candidate_dir / "candidate.f"
    gate = candidate_dir / "hard_gate_report.json"

    if not filelist.exists():
        raise SystemExit("Missing cc_rr_arb_tree candidate.f; copy/build v0.3.1 candidate first.")
    if gate.exists():
        g = json.loads(gate.read_text(encoding="utf-8"))
        overall = g.get("hard_gate", {}).get("overall")
        if overall and overall != "PASS":
            raise SystemExit(f"Compile Hard Gate prerequisite is {overall}, not PASS.")

    verilator = shutil.which("verilator")
    if not verilator:
        raise SystemExit("Verilator not found.")

    out_root = base / args.output_root / args.project / "cc_rr_arb_tree" / args.profile
    out_root.mkdir(parents=True, exist_ok=True)

    print("=== pyCircuit Stateful Correctness v0.4.1 ===")
    print("module        : cc_rr_arb_tree")
    print("profile       :", args.profile)
    print("configs       :", len(configs))
    print("seed          :", hex(args.seed))
    print("CXX           :", args.cxx or "(Verilator configured default)")
    print()

    results = []
    for i, cfg in enumerate(configs):
        seed = (args.seed + i * 0x9E3779B9) & 0xffffffff
        cid = case_id(cfg, seed)
        d = out_root / cid
        if d.exists():
            shutil.rmtree(d)
        d.mkdir(parents=True)

        tb = d / "tb.sv"
        tb.write_text(generate_stateful_tb(cfg, seed), encoding="utf-8")
        obj = d / "obj_dir"

        cmd = [
            verilator, "--binary", "--Wno-fatal",
            "--top-module", "tb",
            "--Mdir", str(obj),
        ]
        if args.cxx:
            cmd += [
                "-MAKEFLAGS", f"CXX={args.cxx}",
                "-MAKEFLAGS", f"LINK={args.cxx}",
            ]
        cmd += [
            "-F", str(filelist),
            str(tb),
        ]

        print(f"[case {i+1}/{len(configs)}] {cfg['name']} mode={cfg['mode']} NumIn={cfg['num_in']}")
        t0 = time.time()
        cp = run(cmd, cwd=d)
        compile_s = time.time() - t0
        (d / "compile_stdout.log").write_text(cp.stdout or "", encoding="utf-8")
        (d / "compile_stderr.log").write_text(cp.stderr or "", encoding="utf-8")

        if cp.returncode != 0:
            result = {
                "name": cfg["name"], "mode": cfg["mode"], "status": "COMPILE_FAIL",
                "config": cfg, "seed": seed, "tests": 0, "errors": 0,
                "compile_returncode": cp.returncode,
                "compile_seconds": round(compile_s, 4),
            }
            results.append(result)
            print("  -> COMPILE_FAIL")
            continue

        sim = obj / "Vtb"
        sp = run([str(sim)], cwd=d)
        (d / "sim_stdout.log").write_text(sp.stdout or "", encoding="utf-8")
        (d / "sim_stderr.log").write_text(sp.stderr or "", encoding="utf-8")

        combined = (sp.stdout or "") + "\n" + (sp.stderr or "")
        matches = list(RESULT_RE.finditer(combined))
        if matches:
            m = matches[-1]
            tests = int(m.group(3))
            errors = int(m.group(4))
            marker_status = m.group(1)
        else:
            tests = 0
            errors = 0
            marker_status = "UNKNOWN"

        status = "PASS" if (sp.returncode == 0 and marker_status == "PASS") else "FAIL"
        result = {
            "name": cfg["name"], "mode": cfg["mode"], "status": status,
            "config": cfg, "seed": seed, "tests": tests, "errors": errors,
            "compile_returncode": cp.returncode,
            "simulation_returncode": sp.returncode,
        }
        results.append(result)
        (d / "case_report.json").write_text(
            json.dumps(result, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8"
        )
        if not args.keep_obj and obj.exists():
            shutil.rmtree(obj)
        print(f"  -> {status} tests={tests} errors={errors}")

    overall = "PASS" if results and all(r["status"] == "PASS" for r in results) else "FAIL"
    report = {
        "schema_version": "0.4.1",
        "module": "cc_rr_arb_tree",
        "profile": args.profile,
        "seed": args.seed,
        "stateful_correctness_gate": overall,
        "configuration_count": len(results),
        "configuration_pass_count": sum(r["status"] == "PASS" for r in results),
        "total_test_steps": sum(r.get("tests", 0) for r in results),
        "total_errors": sum(r.get("errors", 0) for r in results),
        "scope": {
            "ExtPrio": 0,
            "covered_modes": sorted({r["mode"] for r in results}),
            "note": "Internal state/reset/clear/fairness/backpressure/LockIn/AXI-mode behavior by profile."
        },
        "cases": results,
    }
    rp = out_root / "stateful_correctness_report.json"
    rp.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print("\n=== Stateful Correctness Gate ===")
    print("status         :", overall)
    print("configs_passed :", f"{report['configuration_pass_count']}/{len(results)}")
    print("test_steps     :", report["total_test_steps"])
    print("errors         :", report["total_errors"])
    print("report         :", rp)
    raise SystemExit(0 if overall == "PASS" else 1)


if __name__ == "__main__":
    main()
