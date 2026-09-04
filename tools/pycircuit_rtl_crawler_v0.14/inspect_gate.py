#!/usr/bin/env python3
import argparse, json
from pathlib import Path

def main():
    ap = argparse.ArgumentParser(description="Summarize candidate Hard Gate and lint diagnostics")
    ap.add_argument("module")
    ap.add_argument("--project", default="pulp_common_cells")
    ap.add_argument("--candidate-root", default="candidates")
    args = ap.parse_args()

    d = Path(args.candidate_root) / args.project / args.module
    gate = d / "hard_gate_report.json"
    lint = d / "lint_report.json"

    if not gate.exists():
        raise SystemExit(f"Missing {gate}; run build_candidate.py ... --lint first")

    g = json.loads(gate.read_text(encoding="utf-8"))
    print("=== Hard Gate Summary ===")
    print("module              :", g["top_module"])
    print("commit_sha          :", g["commit_sha"])
    print("closure             :", g["hard_gate"]["dependency_closure"])
    print("verilator_compile   :", g["hard_gate"]["verilator_compile"])
    print("overall             :", g["hard_gate"]["overall"])
    print("warnings            :", g["warning_count"])
    print("errors              :", g["error_count"])
    print("warning_codes       :", g["warning_codes"])
    print("error_codes         :", g["error_codes"])

    if lint.exists():
        r = json.loads(lint.read_text(encoding="utf-8"))
        if r.get("warning_records"):
            print("\n--- Warnings ---")
            for w in r["warning_records"]:
                print(f"[{w['code']}] {w['message']}")
        if r.get("error_records"):
            print("\n--- Errors ---")
            for e in r["error_records"]:
                print(f"[{e['code']}] {e['message']}")

if __name__ == "__main__":
    main()
