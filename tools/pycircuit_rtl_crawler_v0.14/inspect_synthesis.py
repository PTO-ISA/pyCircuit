#!/usr/bin/env python3
import argparse, json
from pathlib import Path

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("--profile", default="standard")
    ap.add_argument("--project", default="pulp_common_cells")
    ap.add_argument("--output-root", default="synthesis_results")
    args = ap.parse_args()

    p = Path(args.output_root) / args.project / args.module / args.profile / "synthesis_report.json"
    if not p.exists():
        raise SystemExit(f"Missing {p}")
    r = json.loads(p.read_text(encoding="utf-8"))

    print("=== Synthesis/QoR Summary ===")
    print("module        :", r["module"])
    print("profile       :", r["profile"])
    print("gate          :", r["synthesis_gate"])
    print("yosys         :", r["yosys_version"])
    print("frontend      :", r.get("frontend", "unknown"))
    print("liberty       :", r["liberty"] or "(none)")
    print()
    print("name                         cells      depth      area         status")
    print("-" * 78)
    for c in r["cases"]:
        print(f"{c['name'][:28]:28} "
              f"{str(c.get('num_cells')):10} "
              f"{str(c.get('logic_depth')):10} "
              f"{str(c.get('liberty_area')):12} "
              f"{c.get('generic_status')}")
    print()
    print("Note: logic_depth is a Yosys topological-depth proxy, not timing/Fmax.")

if __name__ == "__main__":
    main()
