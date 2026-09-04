#!/usr/bin/env python3
import csv
import subprocess
import sys
from pathlib import Path

def main():
    out = Path("_smoke_v072")
    if out.exists():
        import shutil
        shutil.rmtree(out)

    p = subprocess.run(
        [sys.executable, "analyze_design_class.py",
         "sample_df09_scaling.csv", "--outdir", str(out)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0, p.stderr

    with (out / "pareto_points.csv").open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    by_n = {}
    for r in rows:
        if r["pareto"] == "True":
            by_n.setdefault(int(r["n"]), []).append(r["project"])

    assert by_n[2] == ["basejump_stl"]
    assert by_n[4] == ["basejump_stl"]
    assert by_n[8] == ["basejump_stl"]
    assert set(by_n[16]) == {"basejump_stl", "opentitan"}

    assert (out / "analysis.html").exists()
    assert (out / "scaling_summary.csv").exists()
    print("smoke_test_v0.7.2: PASS")

if __name__ == "__main__":
    main()
