#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
import yaml

from design_class_adapters import generate_adapter
from design_class_tb import generate_rr_property_tb

def main():
    spec = yaml.safe_load(Path("design_class_specs.yaml").read_text(encoding="utf-8"))
    dc = spec["design_classes"]["DF-09"]
    assert len(dc["candidates"]) == 3

    for c in dc["candidates"]:
        sv = generate_adapter(c["adapter"], 4)
        assert "module pyc_synth_top" in sv
        assert "sel_o" in sv
        assert c["module"] in sv

    tb = generate_rr_property_tb(4, 2)
    assert "PYC_DC_PASS" in tb
    assert "$onehot(sel_o)" in tb
    assert "selection changed under stall" in tb
    assert "incomplete RR round" in tb

    p = subprocess.run(
        [sys.executable, "run_design_class.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--class-id" in p.stdout
    assert "--profile" in p.stdout
    assert "--cxx" in p.stdout

    print("smoke_test_v0.7: PASS")

if __name__ == "__main__":
    main()
