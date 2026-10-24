#!/usr/bin/env python3
# ruff: noqa: T201
"""Independent integer quotient reference for both explicit zero policies."""

import argparse
import json
import random
import subprocess
from pathlib import Path

MASK = (1 << 64) - 1


def reference(a, b, signed, word, ones, valid):
    width = 32 if word else 64
    mask = (1 << width) - 1
    a &= mask
    b &= mask
    if signed:
        a = a - (1 << width) if a >> (width - 1) else a
        b = b - (1 << width) if b >> (width - 1) else b
    if b:
        result = abs(a) // abs(b)
        result = -result if (a < 0) != (b < 0) else result
    else:
        result = -1 if ones else 0
    result &= mask
    if word and result & (1 << 31):
        result |= MASK ^ mask
    return result, valid


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0xD107)
    values = [
        0,
        1,
        2,
        0xFFFFFFFF,
        0x80000000,
        0x7FFFFFFF,
        1 << 63,
        MASK,
        0x123400000000,
    ]
    rows = [
        (a, b, s, w, z, v)
        for a in values
        for b in values
        for s in range(2)
        for w in range(2)
        for z in range(2)
        for v in range(2)
    ]
    rows += [
        (
            rng.getrandbits(64),
            rng.getrandbits(64),
            rng.randrange(2),
            rng.randrange(2),
            rng.randrange(2),
            rng.randrange(2),
        )
        for _ in range(1024)
    ]
    expected = [reference(*r) for r in rows]
    p = subprocess.run(
        [args.runner],
        input="".join(f"{a:x} {b:x} {s} {w} {z} {v}\n" for a, b, s, w, z, v in rows),
        text=True,
        capture_output=True,
        check=True,
    )
    actual = [
        (int(v[0], 16), int(v[1]))
        for line in p.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(rows)
    for row, got, want in zip(rows, actual, expected, strict=True):
        assert got == want, (row, got, want)
    ports = json.loads((args.generated / "ports.json").read_text())
    receipt = json.loads((args.generated / "generated.json").read_text())
    out = ports["outputs"][0]
    decl = []
    connections = []
    samples = []
    for i, port in enumerate(ports["inputs"]):
        decl.append(f"reg[{port['width'] - 1}:0]drive{i};")
        connections.append(f".{port['wire']}(drive{i})")
    decl.append(f"wire[{out['width'] - 1}:0]observed;")
    connections.append(f".{out['wire']}(observed)")
    for index, (row, want) in enumerate(zip(rows, expected, strict=True)):
        a, b, s, w, z, v = row
        values = {
            "lhs": a,
            "rhs": b,
            "is_signed": s,
            "word32": w,
            "zero_ones": z,
            "request_valid": v,
        }
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{values[port['name']]:x};")
        fields = {"result": want[0], "valid": want[1]}
        packed = sum(fields[f["path"][0]] << f["low"] for f in out["fields"])
        samples.append(
            f'#1;if(observed !== {out["width"]}\'h{packed:x})$fatal(1,"quotient {index}");'
        )
    tb = args.output / "tb.sv"
    tb.write_text(
        "module tb;"
        + "".join(decl)
        + receipt["root_rtl_name"]
        + " dut("
        + ",".join(connections)
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
    print(
        f"word_division_oracle rc=0 native_raw_and_valid_matches={len(rows)} rtl_raw_and_valid_matches={len(rows)} zero_policies=2 signed_unsigned=verified widths=32,64 inactive_requests=verified"
    )


if __name__ == "__main__":
    main()
