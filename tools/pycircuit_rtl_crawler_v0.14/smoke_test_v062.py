#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

def main():
    p = subprocess.run(
        [sys.executable, "run_correctness.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--cxx" in p.stdout

    p = subprocess.run(
        [sys.executable, "run_stateful_correctness.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--cxx" in p.stdout

    p = subprocess.run(
        [sys.executable, "run_pipeline.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--cxx" in p.stdout

    rc = Path("run_correctness.py").read_text(encoding="utf-8")
    rs = Path("run_stateful_correctness.py").read_text(encoding="utf-8")
    assert '"-MAKEFLAGS", f"CXX={cxx}"' in rc
    assert '"-MAKEFLAGS", f"LINK={cxx}"' in rc
    assert '"-MAKEFLAGS", f"CXX={args.cxx}"' in rs

    print("smoke_test_v0.6.2: PASS")

if __name__ == "__main__":
    main()
