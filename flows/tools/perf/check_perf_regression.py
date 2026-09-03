#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

TIME_KEYS = ["emit_s", "compile_s", "tb_build_s", "sim_s", "end_to_end_s"]
THROUGHPUT_KEYS = ["cycles_per_sec"]


def _load(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Compare perf-smoke JSON against baseline and fail on regressions."
    )
    ap.add_argument(
        "--current", required=True, help="Current perf JSON from run_perf_smoke.py"
    )
    ap.add_argument("--baseline", required=True, help="Checked baseline JSON")
    ap.add_argument("--time-regression-pct", type=float, default=30.0)
    ap.add_argument("--throughput-regression-pct", type=float, default=30.0)
    args = ap.parse_args()

    current = _load(Path(args.current))
    baseline = _load(Path(args.baseline))

    failures: list[str] = []
    base_cases = baseline.get("cases", {})
    cur_cases = current.get("cases", {})

    time_tol = float(args.time_regression_pct) / 100.0
    thr_tol = float(args.throughput_regression_pct) / 100.0

    for case_name, base_case in base_cases.items():
        cur_case = cur_cases.get(case_name)
        if cur_case is None:
            failures.append(f"{case_name}: missing in current results")
            continue
        if bool(cur_case.get("skipped", False)):
            reason = str(cur_case.get("skip_reason", "unspecified"))
            print(f"perf regression: skipping case {case_name} ({reason})")
            continue

        for key in TIME_KEYS:
            if key not in base_case or key not in cur_case:
                continue
            base_v = float(base_case[key])
            cur_v = float(cur_case[key])
            limit = base_v * (1.0 + time_tol)
            if cur_v > limit:
                failures.append(
                    f"{case_name}: {key} regression {cur_v:.3f}s > {limit:.3f}s "
                    f"(baseline {base_v:.3f}s, tol {args.time_regression_pct:.1f}%)"
                )

        for key in THROUGHPUT_KEYS:
            if key not in base_case or key not in cur_case:
                continue
            base_v = float(base_case[key])
            cur_v = float(cur_case[key])
            floor = base_v * (1.0 - thr_tol)
            if cur_v < floor:
                failures.append(
                    f"{case_name}: {key} regression {cur_v:.2f} < {floor:.2f} "
                    f"(baseline {base_v:.2f}, tol {args.throughput_regression_pct:.1f}%)"
                )

    if failures:
        print("perf regression check FAILED:")
        for f in failures:
            print(f"  - {f}")
        return 1

    print("perf regression check PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
