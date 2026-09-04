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
    ap = argparse.ArgumentParser(description="Run technology-aware DF-09 area benchmark.")
    ap.add_argument("--class-id", default="DF-09")
    ap.add_argument("--profile", choices=["smoke","standard","scaling"], default="scaling")
    ap.add_argument("--liberty", type=Path, required=True)
    ap.add_argument("--cxx", default="/usr/bin/g++-10")
    ap.add_argument("--frontend", choices=["auto","slang","native"], default="auto")
    args = ap.parse_args()

    lib = args.liberty.expanduser().resolve()
    if not lib.exists():
        raise SystemExit(f"Liberty not found: {lib}")

    p = run([sys.executable, "check_liberty.py", str(lib)])
    if p.returncode != 0:
        raise SystemExit("Liberty preflight failed.")

    cmd = [
        sys.executable, "run_design_class.py",
        "--class-id", args.class_id,
        "--profile", args.profile,
        "--cxx", args.cxx,
        "--frontend", args.frontend,
        "--liberty", str(lib),
    ]
    p = run(cmd)
    if p.returncode != 0:
        raise SystemExit("Design-class technology mapping failed.")

    comparison = Path("design_class_results") / args.class_id / args.profile / "comparison.csv"
    p = run([sys.executable, "analyze_mapped_area.py", str(comparison)])
    raise SystemExit(p.returncode)

if __name__ == "__main__":
    main()
