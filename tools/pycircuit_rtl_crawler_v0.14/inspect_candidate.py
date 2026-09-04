#!/usr/bin/env python3
import argparse, csv, json
from pathlib import Path

def load_json(s):
    try:
        return json.loads(s) if s else []
    except Exception:
        return s

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("--output-dir", default="output")
    args = ap.parse_args()

    out = Path(args.output_dir)
    details = out / "candidate_details.csv"
    deps = out / "dependency_edges.csv"

    with details.open(newline="", encoding="utf-8") as f:
        rows = [r for r in csv.DictReader(f) if r.get("module") == args.module]

    if not rows:
        raise SystemExit(f"No exact candidate module: {args.module}")

    for r in rows:
        print("=== Candidate ===")
        for k in ["source_project","repo_url","commit_sha","branch","module","file",
                  "parameter_count","port_count","instance_count",
                  "clock_names","reset_names","handshakes"]:
            print(f"{k:16}: {r.get(k,'')}")

        for label, key in [
            ("Reset info","reset_info_json"),
            ("Parameters","parameters_json"),
            ("Ports","ports_json"),
            ("Instances","instances_json"),
            ("Includes","includes_json"),
            ("Imports","imports_json"),
        ]:
            print(f"\n--- {label} ---")
            print(json.dumps(load_json(r.get(key,"")), indent=2, ensure_ascii=False))

    if deps.exists():
        with deps.open(newline="", encoding="utf-8") as f:
            drows = [r for r in csv.DictReader(f) if r.get("top_module") == args.module]
        print("\n=== Direct Dependencies ===")
        if not drows:
            print("(none detected)")
        for d in drows:
            inst = f" instance={d['instance_name']}" if d.get("instance_name") else ""
            print(f"[{d['resolution_status']}] {d['dependency_kind']}: "
                  f"{d['dependency_name']}{inst} -> {d.get('resolved_file','')}")

if __name__ == "__main__":
    main()
