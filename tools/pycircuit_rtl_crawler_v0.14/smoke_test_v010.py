#!/usr/bin/env python3
from pathlib import Path
import subprocess, sys

def main():
    for f in ["build_runtime_catalog.py", "validate_runtime_catalog.py"]:
        assert Path(f).exists()
    p = subprocess.run([sys.executable, "build_runtime_catalog.py", "--help"], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert p.returncode == 0
    assert "--results-root" in p.stdout
    assert "--liberty" in p.stdout
    print("smoke_test_v0.10: PASS")

if __name__ == "__main__":
    main()
