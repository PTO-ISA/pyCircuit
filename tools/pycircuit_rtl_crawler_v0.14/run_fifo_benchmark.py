#!/usr/bin/env python3
from __future__ import annotations
import argparse
import subprocess
import sys

def main():
    ap = argparse.ArgumentParser(description="Run FIFO-SYNC cross-repository benchmark.")
    ap.add_argument("--profile", choices=["smoke","standard","scaling"], default="smoke")
    ap.add_argument("--workdir", default="work")
    ap.add_argument("--cxx", default="/usr/bin/g++-10")
    ap.add_argument("--liberty", default="")
    ap.add_argument("--frontend", choices=["auto","slang","native"], default="auto")
    args = ap.parse_args()

    cmd = [
        sys.executable, "run_design_class.py",
        "--class-id", "FIFO-SYNC",
        "--profile", args.profile,
        "--workdir", args.workdir,
        "--cxx", args.cxx,
        "--frontend", args.frontend,
    ]
    if args.liberty:
        cmd += ["--liberty", args.liberty]

    print("+", " ".join(cmd))
    raise SystemExit(subprocess.run(cmd).returncode)

if __name__ == "__main__":
    main()
