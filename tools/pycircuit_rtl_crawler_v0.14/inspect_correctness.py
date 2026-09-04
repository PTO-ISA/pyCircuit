#!/usr/bin/env python3
import argparse, json
from pathlib import Path

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("--project", default="pulp_common_cells")
    ap.add_argument("--profile", default="standard")
    ap.add_argument("--output-root", default="correctness_results")
    args = ap.parse_args()

    p = Path(args.output_root) / args.project / args.module / args.profile / "correctness_report.json"
    if not p.exists():
        raise SystemExit(f"Missing {p}")
    r = json.loads(p.read_text(encoding="utf-8"))

    print("=== Correctness Summary ===")
    print("module           :", r["module"])
    print("adapter          :", r["adapter"])
    print("profile          :", r["profile"])
    print("gate             :", r["correctness_gate"])
    print("configs          :", f"{r['configuration_pass_count']}/{r['configuration_count']}")
    print("test_vectors     :", r["total_test_vectors"])
    print("errors           :", r["total_errors"])
    print("verilator        :", r["verilator_version"])
    print("\n--- Scope ---")
    print(json.dumps(r.get("scope", {}), indent=2, ensure_ascii=False))
    print("\n--- Cases ---")
    for c in r["cases"]:
        print(f"[{c['status']}] {c['case_id']} tests={c.get('tests',0)} errors={c.get('errors',0)}")

if __name__ == "__main__":
    main()
