# ruff: noqa: T201
#!/usr/bin/env python3
"""Independent exact-rational reference for every encoded negative binary32 power."""

import argparse
import json
import subprocess
from pathlib import Path

from functional_numeric_oracle import rounded_ratio


def reference(value):
    return 0x7FC00000 if value == 255 else rounded_ratio(0, 1, 1, -value, 8, 23).result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("runner")
    parser.add_argument("generated", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    all_values = range(256)
    p = subprocess.run(
        [args.runner],
        input="".join(f"{v:x}\n" for v in all_values),
        text=True,
        capture_output=True,
        check=True,
    )
    actual = [int(line, 16) for line in p.stdout.splitlines()]
    assert len(actual) == 256
    for value, got in enumerate(actual):
        want = reference(value)
        assert got == want, (value, got, want)
    values = list(range(256))
    receipt = json.loads((args.generated / "generated.json").read_text())
    samples = [
        f"code=8'h{value:x};#1;if(observed !== 32'h{reference(value):x})$fatal(1,\"negative power {i}\");"
        for i, value in enumerate(values)
    ]
    tb = args.output / "tb.sv"
    tb.write_text(
        "module tb;reg[7:0]code;wire[31:0]observed;"
        + receipt["root_rtl_name"]
        + " dut(.code(code),.result(observed));initial begin\n"
        + "\n".join(samples)
        + "\n$finish;end endmodule\n"
    )
    rtl = [args.generated / f["path"] for f in receipt["files"] if f["role"] == "rtl"]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    binary = args.output / "rtl"
    subprocess.run(
        ["iverilog", "-g2012", "-s", "tb", "-o", str(binary), *map(str, rtl), str(tb)],
        check=True,
        capture_output=True,
    )
    subprocess.run(["vvp", str(binary)], check=True, capture_output=True)
    print(
        f"negative_power_oracle rc=0 exhaustive_native_raw_matches=256 rtl_raw_matches={len(values)} normal_subnormal_underflow_and_qnan_sentinel=verified"
    )


if __name__ == "__main__":
    main()
