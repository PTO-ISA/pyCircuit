#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
import yaml

def main():
    reg = yaml.safe_load(Path("benchmark_registry.yaml").read_text(encoding="utf-8"))
    assert "cc_lzc" in reg["designs"]
    assert "cc_popcount" in reg["designs"]
    assert "cc_rr_arb_tree" in reg["designs"]
    assert reg["designs"]["cc_rr_arb_tree"]["stateful"]["enabled"] is True

    p = subprocess.run(
        [sys.executable, "run_pipeline.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--gap-id" in p.stdout
    assert "--all-supported" in p.stdout
    assert "--stages" in p.stdout
    print("smoke_test_v0.6: PASS")

if __name__ == "__main__":
    main()
