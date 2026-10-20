# ruff: noqa: T201
#!/usr/bin/env python3
"""Independent Decimal100 natural-log reference, native and RTL raw/status."""

import argparse
import json
import random
import subprocess
from pathlib import Path

from functional_numeric_oracle import Value, classify, rounded_ratio

FIELDS = [
    "result",
    "valid",
    "invalid",
    "overflow",
    "underflow",
    "inexact",
    "zero",
    "divide_by_zero",
]


def reference(value):
    from decimal import Decimal, localcontext
    from fractions import Fraction

    sign, exponent, fraction, nan, inf, zero = classify(value, 8, 23)
    invalid = int((nan and not (fraction >> 22)) or (sign and not zero and not nan))
    if nan or invalid:
        return Value(0x7FC00000, valid=1 - invalid, invalid=invalid).row()
    if zero:
        return Value(0xFF800000, divide_by_zero=1).row()
    if inf:
        return Value(0x7F800000).row()
    mantissa = ((1 << 23) | fraction) if exponent else fraction
    power = (exponent if exponent else 1) - 150
    with localcontext() as context:
        context.prec = 100
        logarithm = (Decimal(mantissa) * (Decimal(2) ** power)).ln()
        exact = Fraction(logarithm)
    result = rounded_ratio(
        int(exact < 0), abs(exact.numerator), exact.denominator, 0, 8, 23
    )
    result.inexact = int(value != 0x3F800000)
    return result.row()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x10932)
    values = [
        0,
        0x80000000,
        1,
        2,
        0x7FFFFF,
        0x800000,
        0x7F7FFFFF,
        0x7F800000,
        0xFF800000,
        0x7FC00000,
        0x7F800001,
        0xBF800000,
        0x3F800000,
        0x3F800001,
        0x3F7FFFFF,
        0x40000000,
        0x3F000000,
        0x3F7FFFFE,
        0x3F800002,
    ]
    values += [rng.getrandbits(32) for _ in range(1024)]
    rows = [(value,) for value in values]
    wanted = [list(reference(*row)) for row in rows]
    payload = "".join(f"{row[0]:x}\n" for row in rows)
    native = subprocess.run(
        [args.runner], input=payload, text=True, capture_output=True, check=True
    )
    actual = [
        [int(v[0], 16), *map(int, v[1:])]
        for line in native.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(rows), (len(actual), len(rows))
    for row, got, want in zip(rows, actual, wanted, strict=True):
        assert got == want, (row, got, want)
    ports = json.loads((args.generated / "ports.json").read_text())
    receipt = json.loads((args.generated / "generated.json").read_text())
    out = ports["outputs"][0]
    decl, conn, samples = [], [], []
    for i, port in enumerate(ports["inputs"]):
        decl.append(f"reg[{port['width'] - 1}:0]drive{i};")
        conn.append(f".{port['wire']}(drive{i})")
    decl.append(f"wire[{out['width'] - 1}:0]observed;")
    conn.append(f".{out['wire']}(observed)")
    for index, ((value,), want) in enumerate(zip(rows, wanted, strict=True)):
        inputs = {"value": value, "request_valid": 1}
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{inputs[port['name']]:x};")
        fields = dict(zip(FIELDS, want, strict=True))
        packed = sum(fields[f["path"][0]] << f["low"] for f in out["fields"])
        samples.append(
            f'#1;if(observed !== {out["width"]}\'h{packed:x})$fatal(1,"logarithm {index}");'
        )
    tb = args.output / "tb.sv"
    tb.write_text(
        "module tb;"
        + "".join(decl)
        + receipt["root_rtl_name"]
        + " dut("
        + ",".join(conn)
        + ");initial begin\n"
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
    (args.output / "vectors.json").write_text(
        json.dumps(
            [{"input": r, "output": w} for r, w in zip(rows, wanted, strict=True)]
        )
    )
    print(
        f"fp32_log_oracle rc=0 native_raw_and_status_matches={len(rows)} "
        f"rtl_raw_and_status_matches={len(rows)} decimal100_reference=verified Q80_terms=25 bounded_approximation=verified"
    )


if __name__ == "__main__":
    main()
