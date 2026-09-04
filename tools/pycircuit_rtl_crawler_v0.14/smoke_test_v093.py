#!/usr/bin/env python3
from pathlib import Path
import importlib.util

def load(path):
    spec = importlib.util.spec_from_file_location("rt", path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def main():
    rs = Path("run_synthesis.py").read_text(encoding="utf-8")
    assert "write_verilog -noattr -simple-lhs" in rs

    rt = Path("run_timing_benchmark.py").read_text(encoding="utf-8")
    assert "report_checks -path_delay max -group_count 1" in rt
    assert "-endpoint_count" not in rt
    assert "NETLIST_PARSE_FAIL" in rt

    m = load("run_timing_benchmark.py")
    errs = m.opensta_errors("Error: foo\nError: bar\n")
    assert errs == ["foo", "bar"]

    assert Path("check_mapped_netlist.py").exists()
    print("smoke_test_v0.9.3: PASS")

if __name__ == "__main__":
    main()
