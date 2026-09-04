#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

def main():
    src = Path("run_timing_benchmark.py").read_text(encoding="utf-8")
    assert "ThreadPoolExecutor" in src
    assert "--workers" in src
    assert "--timeout-sec" in src
    assert "--only-n" in src
    assert "--resume" in src
    assert "-group_count 1 -endpoint_count 1" in src
    assert "timing_case_report.json" in src

    p = subprocess.run(
        [sys.executable, "run_timing_benchmark.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    for opt in ["--workers", "--timeout-sec", "--only-n", "--resume"]:
        assert opt in p.stdout
    print("smoke_test_v0.9.1: PASS")

if __name__ == "__main__":
    main()
