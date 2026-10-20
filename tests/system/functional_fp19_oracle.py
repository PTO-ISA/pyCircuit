# ruff: noqa: T201
#!/usr/bin/env python3
"""Independent rational reference for every finite-E8M10 sign/exponent/fraction."""

import argparse
import json
import random
import subprocess
from pathlib import Path

from functional_numeric_oracle import rounded_ratio


def reference(value):
    sign = value >> 18
    exponent = (value >> 10) & 255
    fraction = value & 1023
    significand = (1024 | fraction) if exponent else fraction
    power = (exponent if exponent else 1) - 137
    return rounded_ratio(sign, significand, 1, power).result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("runner")
    parser.add_argument("generated", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    all_values = range(1 << 19)
    p = subprocess.run(
        [args.runner],
        input="".join(f"{v:x}\n" for v in all_values),
        text=True,
        capture_output=True,
        check=True,
    )
    actual = [int(line, 16) for line in p.stdout.splitlines()]
    assert len(actual) == 1 << 19
    for value, got in enumerate(actual):
        want = reference(value)
        assert got == want, (value, got, want)
    rng = random.Random(0xF019)
    values = [
        (s << 18) | (e << 10) | f
        for s in range(2)
        for e in range(256)
        for f in (0, 1, 1023)
    ]
    values += [rng.getrandbits(19) for _ in range(4096)]
    receipt = json.loads((args.generated / "generated.json").read_text())
    samples = [
        f"value=19'h{value:x};#1;if(observed !== 64'h{reference(value):x})$fatal(1,\"finite19 {i}\");"
        for i, value in enumerate(values)
    ]
    tb = args.output / "tb.sv"
    tb.write_text(
        "module tb;reg[18:0]value;wire[63:0]observed;"
        + receipt["root_rtl_name"]
        + " dut(.value(value),.result(observed));initial begin\n"
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
        f"fp19_oracle rc=0 exhaustive_native_raw_matches={1 << 19} rtl_raw_matches={len(values)} exponent255_finite=verified signed_zero=verified"
    )


if __name__ == "__main__":
    main()
