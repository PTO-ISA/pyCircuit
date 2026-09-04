#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

def main():
    for f in [
        "check_sta.py",
        "run_timing_benchmark.py",
        "run_area_timing_benchmark.py",
    ]:
        assert Path(f).exists()

    p = subprocess.run(
        [sys.executable, "run_timing_benchmark.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--period-ns" in p.stdout
    assert "--liberty" in p.stdout

    src = Path("run_timing_benchmark.py").read_text(encoding="utf-8")
    assert "set_false_path -from [get_ports rst_ni]" in src
    assert "report_checks -path_delay max" in src
    assert "area_timing_pareto" in src

    print("smoke_test_v0.9: PASS")

if __name__ == "__main__":
    main()
