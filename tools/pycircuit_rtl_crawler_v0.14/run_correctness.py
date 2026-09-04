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
from typing import Dict, List

import yaml

from tb_generators import generate


RESULT_RE = re.compile(
    r"PYC_CORRECTNESS_RESULT\s+(PASS|FAIL)\s+module=([A-Za-z0-9_$]+)\s+tests=(\d+)\s+errors=(\d+)"
)


def load_yaml(path: Path) -> Dict:
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def stable_case_id(module: str, adapter: str, config: Dict, seed: int) -> str:
    payload = json.dumps(
        {"module": module, "adapter": adapter, "config": config, "seed": seed},
        sort_keys=True,
    )
    digest = hashlib.sha1(payload.encode()).hexdigest()[:10]
    parts = [module]
    for k, v in config.items():
        if k == "random_tests":
            continue
        parts.append(f"{k}-{v}")
    return "__".join(parts) + "__" + digest


def run(cmd: List[str], cwd: Path | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def verilator_version() -> str:
    exe = shutil.which("verilator")
    if not exe:
        return ""
    p = run([exe, "--version"])
    return (p.stdout or p.stderr).strip()


def ensure_candidate(
    base: Path,
    module: str,
    project: str,
    candidate_root: Path,
    rebuild: bool,
) -> Path:
    cand = candidate_root / project / module
    filelist = cand / "candidate.f"
    gate = cand / "hard_gate_report.json"

    if rebuild or not filelist.exists():
        cmd = [
            os.environ.get("PYTHON", "python"),
            str(base / "build_candidate.py"),
            module,
            "--project", project,
            "--lint",
        ]
        p = run(cmd, cwd=base)
        if p.returncode != 0:
            raise RuntimeError(
                "build_candidate failed\nSTDOUT:\n{}\nSTDERR:\n{}".format(
                    p.stdout, p.stderr
                )
            )

    if not filelist.exists():
        raise RuntimeError(f"candidate filelist missing: {filelist}")

    if gate.exists():
        data = json.loads(gate.read_text(encoding="utf-8"))
        overall = data.get("hard_gate", {}).get("overall")
        if overall and overall != "PASS":
            raise RuntimeError(
                f"Hard Gate prerequisite is {overall}, not PASS: {gate}"
            )
    return cand


def parse_sim_result(text: str) -> Dict:
    matches = list(RESULT_RE.finditer(text))
    if not matches:
        return {
            "marker_found": False,
            "status": "UNKNOWN",
            "tests": 0,
            "errors": 0,
        }
    m = matches[-1]
    return {
        "marker_found": True,
        "status": m.group(1),
        "module": m.group(2),
        "tests": int(m.group(3)),
        "errors": int(m.group(4)),
    }


def run_case(
    module: str,
    adapter: str,
    config: Dict,
    seed: int,
    candidate_dir: Path,
    out_root: Path,
    keep_obj: bool,
    cxx: str = "",
) -> Dict:
    case_id = stable_case_id(module, adapter, config, seed)
    case_dir = out_root / case_id
    if case_dir.exists():
        shutil.rmtree(case_dir)
    case_dir.mkdir(parents=True)

    tb = case_dir / "tb.sv"
    tb.write_text(generate(adapter, config, seed), encoding="utf-8")

    obj_dir = case_dir / "obj_dir"
    filelist = candidate_dir / "candidate.f"
    verilator = shutil.which("verilator")
    if not verilator:
        return {
            "case_id": case_id,
            "status": "SKIP_TOOL_MISSING",
            "config": config,
            "seed": seed,
        }

    compile_cmd = [
        verilator,
        "--binary",
        "--Wno-fatal",
        "--top-module", "tb",
        "--Mdir", str(obj_dir),
    ]
    if cxx:
        # Verilator --binary invokes GNU Make internally. Pass compiler choices
        # as make command-line variables, which is stronger and more reliable
        # than relying on shell environment variables being honored.
        compile_cmd += [
            "-MAKEFLAGS", f"CXX={cxx}",
            "-MAKEFLAGS", f"LINK={cxx}",
        ]
    compile_cmd += [
        "-F", str(filelist),
        str(tb),
    ]

    t0 = time.time()
    cp = run(compile_cmd, cwd=case_dir)
    compile_s = time.time() - t0

    (case_dir / "compile_stdout.log").write_text(cp.stdout or "", encoding="utf-8")
    (case_dir / "compile_stderr.log").write_text(cp.stderr or "", encoding="utf-8")

    if cp.returncode != 0:
        result = {
            "case_id": case_id,
            "status": "COMPILE_FAIL",
            "config": config,
            "seed": seed,
            "compile_returncode": cp.returncode,
            "compile_seconds": round(compile_s, 4),
            "simulation_returncode": None,
            "tests": 0,
            "errors": 0,
        }
        (case_dir / "case_report.json").write_text(
            json.dumps(result, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )
        return result

    sim = obj_dir / "Vtb"
    if not sim.exists():
        # Fallback for tool/platform naming differences.
        candidates = list(obj_dir.glob("Vtb*"))
        candidates = [p for p in candidates if p.is_file() and os.access(p, os.X_OK)]
        if candidates:
            sim = candidates[0]
        else:
            result = {
                "case_id": case_id,
                "status": "BINARY_MISSING",
                "config": config,
                "seed": seed,
                "compile_returncode": cp.returncode,
                "compile_seconds": round(compile_s, 4),
                "simulation_returncode": None,
                "tests": 0,
                "errors": 0,
            }
            (case_dir / "case_report.json").write_text(
                json.dumps(result, indent=2, ensure_ascii=False) + "\n",
                encoding="utf-8",
            )
            return result

    t1 = time.time()
    sp = run([str(sim)], cwd=case_dir)
    sim_s = time.time() - t1
    stdout = sp.stdout or ""
    stderr = sp.stderr or ""
    combined = stdout + "\n" + stderr
    marker = parse_sim_result(combined)

    (case_dir / "sim_stdout.log").write_text(stdout, encoding="utf-8")
    (case_dir / "sim_stderr.log").write_text(stderr, encoding="utf-8")

    if sp.returncode == 0 and marker["status"] == "PASS":
        status = "PASS"
    else:
        status = "FAIL"

    result = {
        "case_id": case_id,
        "status": status,
        "config": config,
        "seed": seed,
        "compile_returncode": cp.returncode,
        "compile_seconds": round(compile_s, 4),
        "simulation_returncode": sp.returncode,
        "simulation_seconds": round(sim_s, 4),
        "marker_found": marker["marker_found"],
        "tests": marker.get("tests", 0),
        "errors": marker.get("errors", 0),
        "tb_file": str(tb),
    }

    (case_dir / "case_report.json").write_text(
        json.dumps(result, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    if not keep_obj and obj_dir.exists():
        shutil.rmtree(obj_dir)

    return result


def main():
    ap = argparse.ArgumentParser(
        description="pyCircuit Correctness Harness v0.4"
    )
    ap.add_argument("module", choices=["cc_lzc", "cc_popcount", "cc_rr_arb_tree"])
    ap.add_argument("--project", default="pulp_common_cells")
    ap.add_argument("--profile", choices=["smoke", "standard", "full"], default="standard")
    ap.add_argument("--seed", type=lambda x: int(x, 0), default=0x5EED1234)
    ap.add_argument("--specs", default="correctness_specs.yaml")
    ap.add_argument("--candidate-root", default="candidates")
    ap.add_argument("--output-root", default="correctness_results")
    ap.add_argument("--rebuild-candidate", action="store_true")
    ap.add_argument("--keep-obj", action="store_true")
    ap.add_argument(
        "--cxx",
        default="",
        help="C++ compiler used by Verilator internal make, e.g. /usr/bin/g++-10",
    )
    args = ap.parse_args()

    base = Path.cwd()
    specs = load_yaml(base / args.specs)
    module_spec = specs["modules"][args.module]
    adapter = module_spec["adapter"]
    configs = module_spec["profiles"][args.profile]

    candidate_root = (base / args.candidate_root).resolve()
    candidate_dir = ensure_candidate(
        base,
        args.module,
        args.project,
        candidate_root,
        args.rebuild_candidate,
    )

    out_root = (base / args.output_root / args.project / args.module / args.profile).resolve()
    out_root.mkdir(parents=True, exist_ok=True)

    vv = verilator_version()
    print("=== pyCircuit Correctness Harness v0.4 ===")
    print("project       :", args.project)
    print("module        :", args.module)
    print("adapter       :", adapter)
    print("profile       :", args.profile)
    print("configs       :", len(configs))
    print("seed          :", hex(args.seed))
    print("verilator     :", vv or "NOT FOUND")
    print("CXX           :", args.cxx or "(Verilator configured default)")
    print()

    results = []
    total_tests = 0
    total_errors = 0

    for i, config in enumerate(configs):
        case_seed = (args.seed + i * 0x9E3779B9) & 0xFFFFFFFF
        print("[case {}/{}] {}".format(i + 1, len(configs), config))
        result = run_case(
            args.module,
            adapter,
            config,
            case_seed,
            candidate_dir,
            out_root,
            args.keep_obj,
            args.cxx,
        )
        results.append(result)
        total_tests += int(result.get("tests", 0))
        total_errors += int(result.get("errors", 0))
        print("  ->", result["status"],
              "tests=", result.get("tests", 0),
              "errors=", result.get("errors", 0))

    statuses = [r["status"] for r in results]
    overall = "PASS" if statuses and all(s == "PASS" for s in statuses) else "FAIL"
    if any(s == "SKIP_TOOL_MISSING" for s in statuses):
        overall = "SKIP_TOOL_MISSING"

    report = {
        "schema_version": "0.4",
        "source_project": args.project,
        "module": args.module,
        "adapter": adapter,
        "profile": args.profile,
        "seed": args.seed,
        "scope": module_spec.get("scope", {}),
        "verilator_version": vv,
        "cxx": args.cxx,
        "correctness_gate": overall,
        "configuration_count": len(results),
        "configuration_pass_count": sum(r["status"] == "PASS" for r in results),
        "total_test_vectors": total_tests,
        "total_errors": total_errors,
        "cases": results,
    }
    report_path = out_root / "correctness_report.json"
    report_path.write_text(
        json.dumps(report, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    print()
    print("=== Correctness Gate ===")
    print("status           :", overall)
    print("configs_passed   :", f"{report['configuration_pass_count']}/{len(results)}")
    print("test_vectors     :", total_tests)
    print("errors           :", total_errors)
    print("report           :", report_path)

    raise SystemExit(0 if overall == "PASS" else 1)


if __name__ == "__main__":
    main()
