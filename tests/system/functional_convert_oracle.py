#!/usr/bin/env python3
"""Independent exact-rational RNE format-conversion oracle, native and RTL."""

import argparse
import json
import random
import subprocess
from pathlib import Path

from functional_numeric_oracle import FORMATS, Value, classify, rounded_ratio

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


def reference(value, source, destination):
    eb, fb = FORMATS[source]
    sign, exponent, fraction, nan, inf, _ = classify(value, eb, fb)
    de, df = FORMATS[destination]
    if nan:
        signaling = int(not (fraction >> (fb - 1)))
        return Value(
            (((1 << de) - 1) << df) | (1 << (df - 1)),
            valid=1 - signaling,
            invalid=signaling,
        ).row()
    if inf:
        return Value((sign << (de + df)) | (((1 << de) - 1) << df)).row()
    mantissa = ((1 << fb) | fraction) if exponent else fraction
    power = (exponent if exponent else 1) - ((1 << (eb - 1)) - 1) - fb
    return rounded_ratio(sign, mantissa, 1, power, de, df).row()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0xC074)
    rows = []
    for source, (eb, fb) in enumerate(FORMATS):
        inf = ((1 << eb) - 1) << fb
        values = [
            0,
            1 << (eb + fb),
            1,
            (1 << fb) - 1,
            1 << fb,
            inf - 1,
            inf,
            inf | (1 << (eb + fb)),
            inf | 1,
            inf | (1 << (fb - 1)),
            (((1 << (eb - 1)) - 1) << fb),
            (((1 << (eb - 1)) - 1) << fb) | 1,
        ]
        # Midpoints below minimum normal, and direct-conversion double-rounding traps.
        values += {
            0: [0x387FE000, 0x007F8000, 0x477FEFFF, 0x477FF000],
            1: [
                0x380FFFFFE0000000,
                0x3FF0020000000001,
                0x3FF001FFFFFFFFFF,
                0x7FEFFFFFFFFFFFFF,
            ],
        }.get(source, [])
        values += [rng.getrandbits(1 + eb + fb) for _ in range(512)]
        rows += [(value, source, dest) for value in values for dest in range(4)]
    wanted = [list(reference(*row)) for row in rows]
    payload = "".join(f"{a:x} {s} {d}\n" for a, s, d in rows)
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
    for index, ((value, source, destination), want) in enumerate(
        zip(rows, wanted, strict=True)
    ):
        inputs = {
            "value": value,
            "source_format": source,
            "destination_format": destination,
            "request_valid": 1,
        }
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{inputs[port['name']]:x};")
        fields = dict(zip(FIELDS, want, strict=True))
        packed = sum(fields[f["path"][0]] << f["low"] for f in out["fields"])
        samples.append(
            f'#1;if(observed !== {out["width"]}\'h{packed:x})$fatal(1,"format conversion {index}");'
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
    print(  # noqa: T201
        f"float_convert_oracle rc=0 native_raw_and_status_matches={len(rows)} "
        f"rtl_raw_and_status_matches={len(rows)} format_pairs=16 direct_rounding=verified"
    )


if __name__ == "__main__":
    main()
