#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
import yaml

from design_class_adapters import generate_adapter
from design_class_tb import generate_fifo_property_tb

def main():
    spec = yaml.safe_load(Path("design_class_specs.yaml").read_text(encoding="utf-8"))
    dc = spec["design_classes"]["FIFO-SYNC"]
    assert len(dc["candidates"]) == 3
    assert dc["benchmark_kind"] == "fifo_sync"

    cfg = dc["profiles"]["smoke"][0]
    for cand in dc["candidates"]:
        sv = generate_adapter(cand["adapter"], cfg)
        assert "module pyc_synth_top" in sv
        assert cand["module"] in sv
        assert "in_valid_i" in sv
        assert "out_valid_o" in sv

    tb = generate_fifo_property_tb(32, 4, 20)
    assert "PYC_DC_PASS FIFO-SYNC" in tb
    assert "ref_count" in tb
    assert "logical_clear" in tb
    assert "out_data_o !== expected_head" in tb

    p = subprocess.run(
        [sys.executable, "run_fifo_benchmark.py", "--help"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    assert p.returncode == 0
    assert "--workdir" in p.stdout
    assert "--liberty" in p.stdout

    sources = yaml.safe_load(Path("sources.yaml").read_text(encoding="utf-8"))
    bj = next(x for x in sources["sources"] if x["project"] == "basejump_stl")
    assert "bsg_dataflow" in bj["path_hints"]
    assert "bsg_mem" in bj["path_hints"]

    print("smoke_test_v0.11: PASS")

if __name__ == "__main__":
    main()
