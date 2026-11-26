#!/usr/bin/env python3
"""Show why the model reports two roofline verdicts instead of one.

Whitepaper §1 derives the WSE motivation from a balance point and a GEMV
intensity of about ``2 x batch`` FLOP/Byte. That analysis assumes the Cube fills
its ``M x K x N`` block. A tile-accurate model charges for a partly idle ``M``
dimension, and at batch 1 the two answers disagree at FP16. Decision 0006 records
why both are kept; this script prints them side by side.

Run:

    python examples/roofline_two_verdicts.py
"""

from __future__ import annotations

from wse_model.analysis import balance_point, min_batch_to_escape_memory_bound
from wse_model.core import MatmulShape, matmul_timing

K = N = 4096


def line(label: str, shape: MatmulShape, precision_name: str) -> None:
    timing = matmul_timing(shape, precision_name=precision_name)
    report = timing.describe()
    print(
        f"  {label:22s} {precision_name:4s} "
        f"util={report['m_utilization']:<8.4f} "
        f"eff={report['effective_macs_per_cycle']:>8.1f} MAC/c  "
        f"compute={report['compute_seconds'] * 1e6:7.2f}us "
        f"fetch={report['fetch_seconds'] * 1e6:7.2f}us  "
        f"tile-accurate={report['bound_by']:<7s} "
        f"first-order={report['first_order_bound_by']:<7s} "
        f"agree={report['verdicts_agree']}"
    )


def main() -> int:
    print("== balance points (whitepaper §1, §19.2) ==")
    for name in ("fp16", "fp8", "fp4"):
        print(
            f"  {name:4s} {balance_point(name):7.3f} FLOP/Byte   "
            f"min batch {min_batch_to_escape_memory_bound(name):5.2f}"
        )

    print("\n== decode GEMV, batch 1 ==")
    for name in ("fp16", "fp8", "fp4"):
        line("batch 1", MatmulShape(1, K, N), name)

    print("\n== the same weight matrix with a filled M dimension ==")
    for batch in (16, 64):
        for name in ("fp16",):
            line(f"batch {batch}", MatmulShape(batch, K, N), name)

    print(
        "\nReading: at FP16 batch 1 the 16-row Cube block runs 1/16 filled, so the "
        "idle rows, not the memory system, set the time. At FP8/FP4 the memory "
        "side is genuinely the longer leg. Any 'memory-bound' label must say which "
        "verdict it means (decision 0006)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
