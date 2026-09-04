#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

def main():
    for f in [
        "fetch_reference_liberty.py",
        "check_liberty.py",
        "run_technology_benchmark.py",
        "analyze_mapped_area.py",
    ]:
        assert Path(f).exists(), f

    p = subprocess.run(
        [sys.executable, "run_technology_benchmark.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--liberty" in p.stdout
    assert "--profile" in p.stdout

    p = subprocess.run(
        [sys.executable, "fetch_reference_liberty.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "nangate45" in p.stdout

    print("smoke_test_v0.8: PASS")

if __name__ == "__main__":
    main()
