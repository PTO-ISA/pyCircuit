# ruff: noqa: T201
#!/usr/bin/env python3
"""Exact integer product-plus-sum reference with one final rational rounding."""

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


def reference(a, b, c, wide):
    eb, fb = FORMATS[wide]
    aa, bb, cc = [classify(v, eb, fb) for v in (a, b, c)]
    sa, ea, fa, na, ia, za = aa
    sb, _, fbv, nb, ib, zb = bb
    sc, _, fc, nc, ic, _ = cc
    invalid = any(
        nan and not (frac >> (fb - 1)) for _, _, frac, nan, _, _ in (aa, bb, cc)
    )
    invalid |= (ia and zb) or (ib and za)
    pinf = (ia or ib) and not (na or nb)
    invalid |= pinf and ic and ((sa ^ sb) != sc)
    inf = ((1 << eb) - 1) << fb
    if na or nb or nc or invalid:
        return Value(
            inf | (1 << (fb - 1)), valid=int(not invalid), invalid=int(invalid)
        ).row()
    if pinf or ic:
        sign = (sa ^ sb) if pinf else sc
        return Value((sign << (eb + fb)) | inf).row()

    def parts(value):
        sign, ex, frac, *_ = classify(value, eb, fb)
        return (
            sign,
            ((1 << fb) | frac) if ex else frac,
            (ex if ex else 1) - ((1 << (eb - 1)) - 1) - fb,
        )

    _, ma, pa = parts(a)
    _, mb, pb = parts(b)
    _, mc, pc = parts(c)
    common = min(pa + pb, pc)
    total = ((-ma * mb if sa ^ sb else ma * mb) << (pa + pb - common)) + (
        (-mc if sc else mc) << (pc - common)
    )
    sign = int(total < 0) if total else ((sa ^ sb) & sc)
    return rounded_ratio(sign, abs(total), 1, common, eb, fb).row()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0xF053D)
    rows = []
    for wide, (eb, fb) in enumerate(FORMATS[:2]):
        inf = ((1 << eb) - 1) << fb
        sign = 1 << (eb + fb)
        one = ((1 << (eb - 1)) - 1) << fb
        values = [
            0,
            sign,
            1,
            inf - 1,
            inf,
            inf | sign,
            inf | 1,
            inf | (1 << (fb - 1)),
            one,
            one | sign,
            one + 1,
            one - 1,
        ]
        rows += [(a, b, c, wide) for a in values for b in values for c in values]
        rows += [
            (one + 1, one - 2, one | sign, wide),
            (inf - 1, one + 1, (inf - 1) | sign, wide),
        ]
        rows += [
            (
                rng.getrandbits(eb + fb + 1),
                rng.getrandbits(eb + fb + 1),
                rng.getrandbits(eb + fb + 1),
                wide,
            )
            for _ in range(1024)
        ]
    wanted = [list(reference(*row)) for row in rows]
    p = subprocess.run(
        [args.runner],
        input="".join(f"{a:x} {b:x} {c:x} {w}\n" for a, b, c, w in rows),
        text=True,
        capture_output=True,
        check=True,
    )
    actual = [
        [int(v[0], 16), *map(int, v[1:])]
        for line in p.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(rows)
    for row, got, want in zip(rows, actual, wanted, strict=True):
        assert got == want, (row, got, want)
    ports = json.loads((args.generated / "ports.json").read_text())
    receipt = json.loads((args.generated / "generated.json").read_text())
    out = ports["outputs"][0]
    decl = []
    conn = []
    samples = []
    for i, port in enumerate(ports["inputs"]):
        decl.append(f"reg[{port['width'] - 1}:0]drive{i};")
        conn.append(f".{port['wire']}(drive{i})")
    decl.append(f"wire[{out['width'] - 1}:0]observed;")
    conn.append(f".{out['wire']}(observed)")
    for index, ((a, b, c, wide), want) in enumerate(zip(rows, wanted, strict=True)):
        inputs = {"lhs": a, "rhs": b, "addend": c, "wide64": wide, "request_valid": 1}
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{inputs[port['name']]:x};")
        fields = dict(zip(FIELDS, want, strict=True))
        packed = sum(fields[f["path"][0]] << f["low"] for f in out["fields"])
        samples.append(
            f'#1;if(observed !== {out["width"]}\'h{packed:x})$fatal(1,"fused {index}");'
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
        f"float_fused_oracle rc=0 native_raw_and_status_matches={len(rows)} rtl_raw_and_status_matches={len(rows)} binary32_binary64=verified one_round_only=verified"
    )


if __name__ == "__main__":
    main()
