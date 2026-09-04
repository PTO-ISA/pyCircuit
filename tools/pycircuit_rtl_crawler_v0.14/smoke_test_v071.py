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
        assert c["module"] in sv

    # BaseJump width_p must be supplied through the canonical wrapper.
    bsg = generate_adapter("basejump_bsg_rr", 4)
    assert ".width_p ( N )" in bsg

    runner = Path("run_design_class.py").read_text(encoding="utf-8")
    assert "configured_lint(" in runner
    assert '"--top-module", "pyc_synth_top"' in runner
    assert '"--lint"' not in runner.split("build_cmd = [", 1)[1].split("]", 1)[0]

    tb = generate_rr_property_tb(4, 2)
    assert "PYC_DC_PASS" in tb
    assert "$onehot(sel_o)" in tb

    p = subprocess.run(
        [sys.executable, "run_design_class.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0

    print("smoke_test_v0.7.1: PASS")

if __name__ == "__main__":
    main()
