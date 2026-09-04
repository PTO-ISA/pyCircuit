#!/usr/bin/env python3
from pathlib import Path
import importlib.util

def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

def main():
    rs = load("run_synthesis_v052", Path("run_synthesis.py"))
    sw = load("synth_wrapper_v052", Path("synth_wrapper.py"))

    cmd = rs.make_read_command(
        [Path("/tmp/repo/src/cc_pkg.sv"), Path("/tmp/repo/src/cc_lzc.sv")],
        [Path("/tmp/repo/include")],
        Path("/tmp/case/synth_top.sv"),
        "slang",
    )

    assert '"/tmp/' not in cmd
    assert "-I /tmp/repo/include" in cmd
    assert "/tmp/repo/src/cc_pkg.sv" in cmd
    assert "--top pyc_synth_top" in cmd

    wrapper = sw.generate_wrapper("cc_lzc", {"width": 8, "mode": "leading"})
    assert "cc_pkg::idx_width(8)" in wrapper

    print("smoke_test_v0.5.2: PASS")

if __name__ == "__main__":
    main()
