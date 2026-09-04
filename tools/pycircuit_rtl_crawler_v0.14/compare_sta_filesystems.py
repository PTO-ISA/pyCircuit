#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import time
from pathlib import Path

def run_one(sta, tcl, cwd, timeout):
    t0 = time.time()
    p = subprocess.run(
        [sta, str(tcl)],
        cwd=str(cwd),
        text=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        timeout=timeout,
    )
    return p.returncode, time.time() - t0

def main():
    ap = argparse.ArgumentParser(description="Compare OpenSTA runtime on source FS vs Linux scratch.")
    ap.add_argument("--source-tcl", type=Path, required=True)
    ap.add_argument("--scratch-tcl", type=Path, required=True)
    ap.add_argument("--sta", default="sta")
    ap.add_argument("--timeout-sec", type=int, default=60)
    args = ap.parse_args()

    for label, tcl in [("source", args.source_tcl), ("scratch", args.scratch_tcl)]:
        tcl = tcl.resolve()
        try:
            rc, sec = run_one(args.sta, tcl, tcl.parent, args.timeout_sec)
            print(f"{label:8s}: rc={rc} time={sec:.3f}s")
        except subprocess.TimeoutExpired:
            print(f"{label:8s}: TIMEOUT > {args.timeout_sec}s")

if __name__ == "__main__":
    main()
