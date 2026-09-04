#!/usr/bin/env python3
from __future__ import annotations
import argparse
import subprocess
import sys
from pathlib import Path

def run(cmd):
    print("+", " ".join(str(x) for x in cmd))
    return subprocess.run(cmd)

def main():
    ap = argparse.ArgumentParser(description="Run mapped-area then OpenSTA timing benchmark.")
    ap.add_argument("--class-id", default="DF-09")
    ap.add_argument("--profile", choices=["smoke","standard","scaling"], default="scaling")
    ap.add_argument("--liberty", type=Path, required=True)
    ap.add_argument("--cxx", default="/usr/bin/g++-10")
    ap.add_argument("--period-ns", type=float, default=100.0)
    ap.add_argument("--workers", type=int, default=1)
    ap.add_argument("--timeout-sec", type=int, default=600)
    args = ap.parse_args()

    lib = args.liberty.expanduser().resolve()

    p = run([sys.executable, "check_sta.py", str(lib)])
    if p.returncode != 0:
        raise SystemExit("OpenSTA preflight failed.")

    p = run([
        sys.executable, "run_technology_benchmark.py",
        "--class-id", args.class_id,
        "--profile", args.profile,
        "--liberty", str(lib),
        "--cxx", args.cxx,
    ])
    if p.returncode != 0:
        raise SystemExit("Mapped-area benchmark failed.")

    p = run([
        sys.executable, "run_timing_benchmark.py",
        "--class-id", args.class_id,
        "--profile", args.profile,
        "--liberty", str(lib),
        "--period-ns", str(args.period_ns),
        "--workers", str(args.workers),
        "--timeout-sec", str(args.timeout_sec),
    ])
    raise SystemExit(p.returncode)

if __name__ == "__main__":
    main()
