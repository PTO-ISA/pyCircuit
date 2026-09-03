#!/usr/bin/env python3
"""Fix f-string m.input/m.output calls that lack {prefix}_ prefix.

Transforms:
  m.input(f"NAME_{var}", ...) → m.input(f"{prefix}_NAME_{var}", ...)
  m.output(f"NAME_{var}", ...) → m.output(f"{prefix}_NAME_{var}", ...)

Only modifies calls inside build_* function bodies.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

XS_ROOT = Path(__file__).resolve().parent.parent


def fix_file(filepath: Path, *, dry_run: bool = False) -> int:
    text = filepath.read_text()
    original = text

    # Match m.input(f"SOMETHING" or m.output(f"SOMETHING" where SOMETHING
    # does NOT start with {prefix}_
    pat = re.compile(r'(m\.(input|output)\(f")(?!\{prefix\}_)')

    def replacer(match):
        return match.group(1) + "{prefix}_"

    # Only apply inside build_* function bodies
    # Find all build_* functions and apply within their bodies
    func_pat = re.compile(r"^def (build_\w+)\(", re.MULTILINE)
    last_end = 0
    parts = []

    for func_m in func_pat.finditer(text):
        # Add everything before this function unchanged
        parts.append(text[last_end : func_m.start()])

        func_start = func_m.start()
        # Find the end of this function
        next_toplevel = re.compile(
            r"^(?:def |class |[a-zA-Z_]\w*\.\w|if __name__|@)", re.MULTILINE
        )
        first_nl = text.index("\n", func_start)
        next_match = None
        for it in next_toplevel.finditer(text, first_nl + 1):
            if it.start() > func_start + 10:
                next_match = it
                break
        func_end = next_match.start() if next_match else len(text)

        func_body = text[func_start:func_end]
        func_body = pat.sub(replacer, func_body)
        parts.append(func_body)
        last_end = func_end

    parts.append(text[last_end:])
    text = "".join(parts)

    count = len(text) - len(original)  # rough count
    if text == original:
        return 0

    # Count actual replacements
    n = sum(1 for _ in pat.finditer(original))  # Not quite right, but close enough
    actual = text.count("{prefix}_") - original.count("{prefix}_")

    if dry_run:
        print(f"  DRY  {filepath.relative_to(XS_ROOT)}: +{actual} prefix insertions")
    else:
        filepath.write_text(text)
        print(f"  WRITE {filepath.relative_to(XS_ROOT)}: +{actual} prefix insertions")
    return actual


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    total = 0
    for py_file in sorted(XS_ROOT.rglob("*.py")):
        if py_file.name.startswith(("test_", "tb_", "sva_")):
            continue
        if py_file.name == "build_verilog.py":
            continue
        if "tools/" in str(py_file.relative_to(XS_ROOT)):
            continue
        n = fix_file(py_file, dry_run=args.dry_run)
        total += n

    print(f"\nTotal: {total} prefix insertions")
    if args.dry_run:
        print("(DRY RUN)")


if __name__ == "__main__":
    main()
