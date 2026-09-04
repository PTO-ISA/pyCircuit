#!/usr/bin/env python3
from __future__ import annotations
import argparse
import re
from pathlib import Path

PATTERNS = [
    (re.compile(r"\binitial\b"), "initial"),
    (re.compile(r"\bassert\s*\("), "assert"),
    (re.compile(r"\bassume\s*\("), "assume"),
    (re.compile(r"\bcover\s*\("), "cover"),
    (re.compile(r"\brestrict\s*\("), "restrict"),
]

def inspect(path: Path):
    text = path.read_text(encoding="utf-8", errors="replace")
    hits = []
    for rx, kind in PATTERNS:
        for m in rx.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            hits.append((line, kind))
    return sorted(hits)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("netlist", type=Path)
    args = ap.parse_args()

    if not args.netlist.exists():
        raise SystemExit(f"NETLIST_MISSING: {args.netlist}")

    hits = inspect(args.netlist)
    if hits:
        print("QOR_NETLIST_SANITATION_FAIL")
        for line, kind in hits[:50]:
            print(f"line {line}: residual {kind}")
        raise SystemExit(2)

    print("QOR_NETLIST_SANITATION_PASS")

if __name__ == "__main__":
    main()
