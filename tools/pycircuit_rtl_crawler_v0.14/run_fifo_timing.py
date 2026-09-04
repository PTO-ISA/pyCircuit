#!/usr/bin/env python3
from __future__ import annotations
import argparse
import subprocess
import sys

def main():
    ap = argparse.ArgumentParser(description="Run FIFO-SYNC OpenSTA benchmark.")
    ap.add_argument("--profile", choices=["smoke","standard","scaling"], default="scaling")
    ap.add_argument("--results-root", default="design_class_results")
    ap.add_argument("--liberty", required=True)
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--timeout-sec", type=int, default=60)
    ap.add_argument("--only-config", action="append", default=[])
    ap.add_argument("--resume", action="store_true")
    args = ap.parse_args()

    cmd = [
        sys.executable,
        "run_timing_benchmark.py",
        "--class-id", "FIFO-SYNC",
        "--profile", args.profile,
        "--results-root", args.results_root,
        "--liberty", args.liberty,
        "--workers", str(args.workers),
        "--timeout-sec", str(args.timeout_sec),
    ]
    for cfg in args.only_config:
        cmd += ["--only-config", cfg]
    if args.resume:
        cmd += ["--resume"]

    print("+", " ".join(cmd))
    raise SystemExit(subprocess.run(cmd).returncode)

if __name__ == "__main__":
    main()
