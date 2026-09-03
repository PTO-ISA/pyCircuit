#!/usr/bin/env python3
"""Batch-add `prefix` parameter to all build_* functions in XiangShan-pyc.

Transforms:
  - def build_xxx(m, domain, *, ...) → def build_xxx(m, domain, *, prefix="xxx", ...)
  - m.input("name", ...) → m.input(f"{prefix}_name", ...)
  - m.output("name", ...) → m.output(f"{prefix}_name", ...)
  - domain.signal(..., name="name") → domain.signal(..., name=f"{prefix}_name")
  - domain.cycle(..., name="name") → domain.cycle(..., name=f"{prefix}_name")

Usage: python tools/add_prefix.py [--dry-run] [--file path]
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

XS_ROOT = Path(__file__).resolve().parent.parent

ALL_MODULES: list[tuple[str, str]] = [
    # (relative_path, default_prefix)
    ("backend/exu/alu.py", "alu"),
    ("backend/exu/bru.py", "bru"),
    ("backend/exu/mul.py", "mul"),
    ("backend/exu/div.py", "div"),
    ("backend/fu/fpu.py", "fpu"),
    ("backend/regfile/regfile.py", "rf"),
    ("backend/rename/rename.py", "ren"),
    ("backend/dispatch/dispatch.py", "dp"),
    ("backend/issue/issue_queue.py", "iq"),
    ("backend/rob/rob.py", "rob"),
    ("backend/ctrlblock/ctrlblock.py", "ctrl"),
    ("backend/backend.py", "be"),
    ("frontend/bpu/ubtb.py", "ubtb"),
    ("frontend/bpu/tage.py", "tage"),
    ("frontend/bpu/sc.py", "sc"),
    ("frontend/bpu/ittage.py", "ittage"),
    ("frontend/bpu/ras.py", "ras"),
    ("frontend/bpu/bpu.py", "bpu"),
    ("frontend/ftq/ftq.py", "ftq"),
    ("frontend/icache/icache.py", "ic"),
    ("frontend/ifu/ifu.py", "ifu"),
    ("frontend/ibuffer/ibuffer.py", "ibuf"),
    ("frontend/decode/decode.py", "dec"),
    ("frontend/frontend.py", "fe"),
    ("cache/dcache/dcache.py", "dc"),
    ("cache/mmu/tlb.py", "tlb"),
    ("mem/pipeline/load_unit.py", "ldu"),
    ("mem/pipeline/store_unit.py", "stu"),
    ("mem/lsqueue/load_queue.py", "ldq"),
    ("mem/lsqueue/store_queue.py", "stq"),
    ("mem/sbuffer/sbuffer.py", "sbuf"),
    ("mem/prefetch/prefetcher.py", "pf"),
    ("mem/memblock.py", "mem"),
    ("l2/l2_top.py", "l2top"),
    ("l2/coupled_l2.py", "l2"),
    ("top/peripherals.py", "plic"),  # has two functions, handled specially
    ("top/xs_core.py", "core"),
    ("top/xs_tile.py", "tile"),
    ("top/xs_top.py", "soc"),
]

PERIPHERALS_FUNCS = {
    "build_plic": "plic",
    "build_clint": "clint",
}


def transform_file(
    filepath: Path, default_prefix: str, *, dry_run: bool = False
) -> dict:
    """Transform a single file. Returns stats dict."""
    text = filepath.read_text()
    original = text
    stats = {"input": 0, "output": 0, "state": 0, "cycle": 0, "sig": 0}

    is_peripherals = filepath.name == "peripherals.py"

    if is_peripherals:
        for func_name, pfx in PERIPHERALS_FUNCS.items():
            text = _transform_function_in_text(text, func_name, pfx, stats)
    else:
        func_match = re.search(r"def (build_\w+)\(", text)
        if not func_match:
            return stats
        func_name = func_match.group(1)
        text = _transform_function_in_text(text, func_name, default_prefix, stats)

    if text == original:
        return stats

    if dry_run:
        pass
    else:
        filepath.write_text(text)

    return stats


def _transform_function_in_text(
    text: str, func_name: str, prefix: str, stats: dict
) -> str:
    """Add prefix param and transform m.input/m.output/domain.signal/domain.cycle."""

    # 1) Add prefix parameter to function signature
    # Match: def build_xxx(\n    m: ...,\n    domain: ...,\n    *,\n
    sig_pattern = re.compile(
        rf"(def {re.escape(func_name)}\(\s*\n\s*m:\s*CycleAwareCircuit,\s*\n"
        rf"\s*domain:\s*CycleAwareDomain,\s*\n\s*\*,\s*\n)"
    )
    sig_match = sig_pattern.search(text)
    if sig_match:
        insertion = sig_match.group(1)
        indent = "    "
        new_sig = insertion + f'{indent}prefix: str = "{prefix}",\n'
        text = text[: sig_match.start()] + new_sig + text[sig_match.end() :]
        stats["sig"] += 1
    else:
        sig_pattern2 = re.compile(
            rf"(def {re.escape(func_name)}\(\s*m:\s*CycleAwareCircuit,\s*"
            rf"domain:\s*CycleAwareDomain,\s*\*,\s*)"
        )
        sig_match2 = sig_pattern2.search(text)
        if sig_match2:
            insertion = sig_match2.group(1)
            new_sig = insertion + f'prefix: str = "{prefix}", '
            text = text[: sig_match2.start()] + new_sig + text[sig_match2.end() :]
            stats["sig"] += 1

    # Find the function body range (from def to next top-level statement)
    func_def_pattern = re.compile(rf"^def {re.escape(func_name)}\(", re.MULTILINE)
    func_start_match = func_def_pattern.search(text)
    if not func_start_match:
        return text
    func_start = func_start_match.start()

    # Find end of function: next top-level non-indented code that isn't a comment/blank
    # Look for: next def, variable assignment (build_xxx.), if __name__, or class
    next_toplevel = re.compile(
        r"^(?:def |class |[a-zA-Z_]\w*\.\w|if __name__|@)", re.MULTILINE
    )
    # Start searching after the first line of the function
    first_newline = text.index("\n", func_start)
    next_match = None
    for m_iter in next_toplevel.finditer(text, first_newline + 1):
        if m_iter.start() > func_start + 10:
            next_match = m_iter
            break
    func_end = next_match.start() if next_match else len(text)

    before = text[:func_start]
    body = text[func_start:func_end]
    after = text[func_end:]

    # 2) Transform m.input("name" → m.input(f"{prefix}_name"
    def replace_input(match):
        name = match.group(1)
        stats["input"] += 1
        return f'm.input(f"{{prefix}}_{name}"'

    body = re.sub(r'm\.input\("([^"]+)"', replace_input, body)

    # 3) Transform m.output("name" → m.output(f"{prefix}_name"
    def replace_output(match):
        name = match.group(1)
        stats["output"] += 1
        return f'm.output(f"{{prefix}}_{name}"'

    body = re.sub(r'm\.output\("([^"]+)"', replace_output, body)

    # 4) Transform name="literal" → name=f"{prefix}_literal"
    #    AND       name=f"literal_{var}" → name=f"{prefix}_literal_{var}"
    #    in domain.signal / domain.cycle calls
    def replace_name_literal(match):
        name = match.group(1)
        stats["state"] += 1
        return f'name=f"{{prefix}}_{name}"'

    body = re.sub(r'name="([^"]+)"(?=\s*\))', replace_name_literal, body)

    def replace_name_fstring(match):
        inner = match.group(1)
        if inner.startswith("{prefix}_"):
            return match.group(0)  # already transformed, skip
        stats["cycle"] += 1
        return f'name=f"{{prefix}}_{inner}"'

    body = re.sub(r'name=f"([^"]+)"(?=\s*\))', replace_name_fstring, body)

    return before + body + after


def main():
    parser = argparse.ArgumentParser(description="Add prefix to build_* functions")
    parser.add_argument(
        "--dry-run", action="store_true", help="Show changes without writing"
    )
    parser.add_argument(
        "--file",
        type=str,
        default=None,
        help="Process a single file (relative to XiangShan-pyc/)",
    )
    args = parser.parse_args()

    total_stats = {"input": 0, "output": 0, "state": 0, "cycle": 0, "sig": 0}

    if args.file:
        modules = [(m, p) for m, p in ALL_MODULES if m == args.file]
        if not modules:
            return 1
    else:
        modules = ALL_MODULES

    processed = 0
    for rel_path, default_prefix in modules:
        filepath = XS_ROOT / rel_path
        if not filepath.exists():
            continue
        s = transform_file(filepath, default_prefix, dry_run=args.dry_run)
        for k in total_stats:
            total_stats[k] += s[k]
        processed += 1

    if args.dry_run:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
