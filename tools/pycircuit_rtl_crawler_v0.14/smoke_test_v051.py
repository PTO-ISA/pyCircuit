#!/usr/bin/env python3
from pathlib import Path
import importlib.util

def main():
    p = Path("run_synthesis.py")
    spec = importlib.util.spec_from_file_location("run_synthesis_v051", p)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

    srcs = [Path("/tmp/a.sv"), Path("/tmp/b.sv")]
    incs = [Path("/tmp/include")]
    wrapper = Path("/tmp/top.sv")

    slang = mod.make_read_command(srcs, incs, wrapper, "slang")
    assert slang.startswith("read_slang ")
    assert "--std 1800-2023" in slang
    assert "--single-unit" in slang
    assert "--top pyc_synth_top" in slang
    assert "-I" in slang

    native = mod.make_read_command(srcs, incs, wrapper, "native")
    assert native.startswith("read_verilog -sv ")

    print("smoke_test_v0.5.1: PASS")

if __name__ == "__main__":
    main()
