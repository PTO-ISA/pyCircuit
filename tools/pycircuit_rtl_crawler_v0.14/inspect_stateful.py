#!/usr/bin/env python3
import argparse, json
from pathlib import Path

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--profile", default="standard")
    ap.add_argument("--project", default="pulp_common_cells")
    ap.add_argument("--output-root", default="stateful_results")
    args = ap.parse_args()

    p = Path(args.output_root) / args.project / "cc_rr_arb_tree" / args.profile / "stateful_correctness_report.json"
    if not p.exists():
        raise SystemExit(f"Missing {p}")
    r = json.loads(p.read_text(encoding="utf-8"))

    print("=== Stateful Correctness Summary ===")
    print("module           :", r["module"])
    print("profile          :", r["profile"])
    print("gate             :", r["stateful_correctness_gate"])
    print("configs          :", f"{r['configuration_pass_count']}/{r['configuration_count']}")
    print("test_steps       :", r["total_test_steps"])
    print("errors           :", r["total_errors"])
    print("\n--- Cases ---")
    for c in r["cases"]:
        print(f"[{c['status']}] {c['name']} mode={c['mode']} tests={c.get('tests',0)} errors={c.get('errors',0)}")

if __name__ == "__main__":
    main()
