#!/usr/bin/env python3
from pathlib import Path
import importlib.util
import tempfile

def load(path):
    spec = importlib.util.spec_from_file_location("rt", path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def main():
    m = load("run_timing_benchmark.py")
    src = Path("sample_df09_scaling.csv").resolve()

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        staged = m.stage_liberty_to_linux_fs(src, td)
        assert staged.exists()
        assert staged.read_bytes() == src.read_bytes()

        case = m.stage_case_netlist(src, td, "p", "m", "n16")
        assert case.exists()
        assert case.read_bytes() == src.read_bytes()

    text = Path("run_timing_benchmark.py").read_text(encoding="utf-8")
    assert "--scratch-root" in text
    assert "--no-local-stage" in text
    assert "stage_liberty_to_linux_fs" in text
    assert "stage_case_netlist" in text
    print("smoke_test_v0.9.4: PASS")

if __name__ == "__main__":
    main()
