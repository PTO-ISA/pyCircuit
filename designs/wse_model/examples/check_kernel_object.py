#!/usr/bin/env python3
"""Check a compiled kernel object against the F1/F2/F3 prohibitions.

Whitepaper §10.2 and Calendar §3.10: once the design removed the "one
instruction atomically selects route and timeslot" mechanism, these three
prohibitions became the product's only consistency anchor. The model checks a
declared symbol/section manifest rather than parsing ELF, so the invariant is
testable without a compiler.

Run:

    python examples/check_kernel_object.py [path/to/object.json]

With no argument it checks the conforming baseline in
``examples/data/ffn-kernel-object.json`` and then a deliberately broken copy of
it, so both outcomes are visible.
"""

from __future__ import annotations

import copy
import json
import sys
from pathlib import Path

from wse_model.compiler import check_compiled_product, load_object_file

DATA = Path(__file__).resolve().parent / "data" / "ffn-kernel-object.json"
KEY_COUNT = 2
NODE_COUNT = 40


def _check(label: str, payload: dict) -> bool:
    obj = load_object_file(payload)
    report = check_compiled_product(obj, key_count=KEY_COUNT, node_count=NODE_COUNT)
    print(f"\n== {label} ==")
    for symbol in obj.symbols:
        print(
            f"  {symbol.name:16s} {symbol.kind.value:18s} "
            f"{symbol.section.value:8s} size={symbol.size:<6d} align={symbol.addr_align}"
        )
    print(report.format())
    return report.ok


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)

    if argv:
        payload = json.loads(Path(argv[0]).read_text(encoding="utf-8"))
        return 0 if _check(str(argv[0]), payload) else 1

    good = json.loads(DATA.read_text(encoding="utf-8"))
    ok_good = _check("conforming FFN kernel object", good)

    # F1: a writable Calendar global. Under SPMD all 40 cores would race on it.
    broken_f1 = copy.deepcopy(good)
    broken_f1["symbols"].append(
        {
            "name": "CalendarChannel",
            "kind": "writable object",
            "section": ".data",
            "size": 32,
        }
    )
    ok_f1 = _check("F1 violation: writable CalendarChannel in .data", broken_f1)

    # F2: a materialized CalendarKeyRef constant.
    broken_f2 = copy.deepcopy(good)
    broken_f2["symbols"].append(
        {
            "name": "kRowAllGather",
            "kind": "read-only object",
            "section": ".rodata",
            "size": 16,
            "addr_align": 16,
        }
    )
    ok_f2 = _check("F2 violation: materialized kRowAllGather", broken_f2)

    # F3: the route table dropped to 32 B alignment.
    broken_f3 = copy.deepcopy(good)
    broken_f3["sections"][1]["addr_align"] = 32
    ok_f3 = _check("F3 violation: .rodata at 32 B alignment", broken_f3)

    print(
        "\nresult: conforming "
        f"{'passes' if ok_good else 'FAILS'}; each violation "
        f"{'passes (wrong!)' if ok_f1 or ok_f2 or ok_f3 else 'is rejected as required'}"
    )
    return 0 if ok_good and not (ok_f1 or ok_f2 or ok_f3) else 1


if __name__ == "__main__":
    raise SystemExit(main())
