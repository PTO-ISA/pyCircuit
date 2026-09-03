#!/usr/bin/env python3
"""Batch-refactor all build_* functions for explicit signal passing.

Transforms each build_* to:
  1. Add `inputs: dict[str, CycleAwareSignal] | None = None` parameter
  2. Add `_in = inputs or {}` and `_out = {}` at function body start
  3. Change `m.input(f"{prefix}_NAME", ...)` → dual-mode with `_in["NAME"]`
  4. After `m.output(f"{prefix}_NAME", wire_of(VAR))` → add `_out["NAME"] = VAR`
  5. Change return type to `-> dict[str, CycleAwareSignal]:`
  6. Add `return _out` before function end

Usage: python tools/add_explicit_signals.py [--dry-run] [--file path]
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

XS_ROOT = Path(__file__).resolve().parent.parent

ALL_MODULES: list[tuple[str, str]] = [
    ("backend/exu/alu.py", "build_alu"),
    ("backend/exu/bru.py", "build_bru"),
    ("backend/exu/mul.py", "build_mul"),
    ("backend/exu/div.py", "build_div"),
    ("backend/fu/fpu.py", "build_fpu"),
    ("backend/regfile/regfile.py", "build_regfile"),
    ("backend/rename/rename.py", "build_rename"),
    ("backend/dispatch/dispatch.py", "dispatch"),
    ("backend/issue/issue_queue.py", "build_issue_queue"),
    ("backend/rob/rob.py", "build_rob"),
    ("backend/ctrlblock/ctrlblock.py", "build_ctrlblock"),
    ("backend/backend.py", "build_backend"),
    ("frontend/bpu/ubtb.py", "build_ubtb"),
    ("frontend/bpu/tage.py", "build_tage"),
    ("frontend/bpu/sc.py", "build_sc"),
    ("frontend/bpu/ittage.py", "build_ittage"),
    ("frontend/bpu/ras.py", "build_ras"),
    ("frontend/bpu/bpu.py", "build_bpu"),
    ("frontend/ftq/ftq.py", "build_ftq"),
    ("frontend/icache/icache.py", "build_icache"),
    ("frontend/ifu/ifu.py", "build_ifu"),
    ("frontend/ibuffer/ibuffer.py", "build_ibuffer"),
    ("frontend/decode/decode.py", "build_decode"),
    ("frontend/frontend.py", "build_frontend"),
    ("cache/dcache/dcache.py", "build_dcache"),
    ("cache/mmu/tlb.py", "build_tlb"),
    ("mem/pipeline/load_unit.py", "build_load_unit"),
    ("mem/pipeline/store_unit.py", "build_store_unit"),
    ("mem/lsqueue/load_queue.py", "build_load_queue"),
    ("mem/lsqueue/store_queue.py", "build_store_queue"),
    ("mem/sbuffer/sbuffer.py", "build_sbuffer"),
    ("mem/prefetch/prefetcher.py", "build_prefetcher"),
    ("mem/memblock.py", "build_memblock"),
    ("l2/l2_top.py", "build_l2_top"),
    ("l2/coupled_l2.py", "build_coupled_l2"),
    ("top/peripherals.py", "build_plic"),
    ("top/peripherals.py", "build_clint"),
    ("top/xs_core.py", "build_xs_core"),
    ("top/xs_tile.py", "build_xs_tile"),
    ("top/xs_top.py", "build_xs_top"),
]


def transform_file(filepath: Path, func_name: str, *, dry_run: bool = False) -> dict:
    text = filepath.read_text()
    original = text
    stats = {"sig": 0, "init": 0, "input": 0, "output": 0, "ret": 0}

    text = _transform_function(text, func_name, stats)

    if text == original:
        return stats

    if dry_run:
        pass
    else:
        filepath.write_text(text)

    return stats


def _find_func_range(text: str, func_name: str):
    """Return (func_start, body_start, func_end) character offsets."""
    func_pat = re.compile(rf"^def {re.escape(func_name)}\(", re.MULTILINE)
    m = func_pat.search(text)
    if not m:
        return None

    func_start = m.start()

    # Find end of signature: `) -> ...:` or just `):` with possible multiline
    sig_end_pat = re.compile(r"\)\s*(?:->.*?)?:\s*\n", re.DOTALL)
    sig_m = sig_end_pat.search(text, func_start)
    if not sig_m:
        return None
    body_start = sig_m.end()

    # Skip docstring if present
    after_sig = text[body_start:].lstrip()
    if after_sig.startswith('"""') or after_sig.startswith("'''"):
        quote = after_sig[:3]
        ds_start = text.index(quote, body_start)
        ds_end = text.index(quote, ds_start + 3) + 3
        # Skip to next line
        nl = text.index("\n", ds_end)
        body_start = nl + 1

    # Find function end
    next_toplevel = re.compile(
        r"^(?:def |class |[a-zA-Z_]\w*\.\w|if __name__|@)", re.MULTILINE
    )
    first_newline = text.index("\n", func_start)
    next_match = None
    for it in next_toplevel.finditer(text, first_newline + 1):
        if it.start() > func_start + 10:
            next_match = it
            break
    func_end = next_match.start() if next_match else len(text)

    return func_start, body_start, func_end


def _transform_function(text: str, func_name: str, stats: dict) -> str:
    rng = _find_func_range(text, func_name)
    if not rng:
        return text
    func_start, body_start, func_end = rng

    # Separate signature from docstring
    sig_end_pat = re.compile(r"\)\s*(?:->.*?)?:\s*\n", re.DOTALL)
    sig_end_m = sig_end_pat.search(text, func_start)
    sig_boundary = sig_end_m.end() if sig_end_m else body_start

    sig_part = text[func_start:sig_boundary]
    doc_part = text[sig_boundary:body_start]
    body = text[body_start:func_end]
    before = text[:func_start]
    after = text[func_end:]

    # 1. Add `inputs` parameter and change return type (only on signature)
    sig_part, sig_changed = _add_inputs_param(sig_part, func_name)
    if sig_changed:
        stats["sig"] += 1

    sig_and_doc = sig_part + doc_part

    # 2. Add _in/_out init at body start
    if "_in = inputs or {}" not in body:
        indent = "    "
        init_block = (
            f"{indent}_in = inputs or {{}}\n"
            f"{indent}_out: dict[str, CycleAwareSignal] = {{}}\n\n"
        )
        body = init_block + body
        stats["init"] += 1

    # 3. Transform m.input patterns → dual-mode
    body, n_in = _transform_inputs(body)
    stats["input"] += n_in

    # 4. Transform m.output patterns → add _out collection
    body, n_out = _transform_outputs(body)
    stats["output"] += n_out

    # 5. Add return _out at end of function
    if "return _out" not in body:
        # Find the last non-empty line and add return after it
        lines = body.rstrip().split("\n")
        lines.append("    return _out\n")
        body = "\n".join(lines) + "\n\n"
        stats["ret"] += 1

    return before + sig_and_doc + body + after


def _add_inputs_param(sig: str, func_name: str) -> tuple[str, bool]:
    """Add inputs param and change -> None to -> dict[str, CycleAwareSignal]."""
    changed = False

    # Add inputs parameter (before the closing `) -> ...`)
    if "inputs:" not in sig and "inputs :" not in sig:
        # Find the last parameter line before `) -> None:`
        # Pattern: last `keyword: type = default,\n` before `)`
        close_paren = sig.rfind(")")
        if close_paren > 0:
            # Insert before the closing paren
            insert_pos = close_paren
            # Find proper indentation
            last_nl = sig.rfind("\n", 0, close_paren)
            if last_nl > 0:
                indent_match = re.match(r"(\s+)", sig[last_nl + 1 :])
                indent = indent_match.group(1) if indent_match else "    "
            else:
                indent = "    "
            insertion = f"{indent}inputs: dict[str, CycleAwareSignal] | None = None,\n"
            sig = sig[:insert_pos] + insertion + sig[insert_pos:]
            changed = True

    # Change -> None to -> dict[str, CycleAwareSignal]
    if "-> None:" in sig:
        sig = sig.replace("-> None:", "-> dict[str, CycleAwareSignal]:")
        changed = True

    return sig, changed


def _transform_inputs(body: str) -> tuple[str, int]:
    """Transform `x = cas(domain, m.input(f"{prefix}_NAME", ...), cycle=C)`
    to dual-mode with _in["NAME"] check."""
    count = 0

    # Pattern: VAR = cas(domain, m.input(f"{prefix}_NAME", width=EXPR), cycle=NUM)
    pattern = re.compile(
        r'^(\s+)(\w+)\s*=\s*cas\(domain,\s*m\.input\(f"\{prefix\}_(\w+)",\s*width=([^)]+)\),\s*cycle=(\d+)\)',
        re.MULTILINE,
    )

    def replacer(match):
        nonlocal count
        indent = match.group(1)
        var = match.group(2)
        name = match.group(3)
        width = match.group(4)
        cycle = match.group(5)
        count += 1
        return (
            f'{indent}{var} = (_in["{name}"] if "{name}" in _in else\n'
            f'{indent}    cas(domain, m.input(f"{{prefix}}_{name}", width={width}), cycle={cycle}))'
        )

    body = pattern.sub(replacer, body)
    return body, count


def _transform_outputs(body: str) -> tuple[str, int]:
    """After `m.output(f"{prefix}_NAME", wire_of(VAR))`, add `_out["NAME"] = VAR`.
    For raw wire outputs (no wire_of), wrap in note."""
    count = 0
    lines = body.split("\n")
    new_lines = []

    # Pattern 1: m.output(f"{prefix}_NAME", wire_of(VAR))
    pat_wire = re.compile(r'^(\s+)m\.output\(f"\{prefix\}_(\w+)",\s*wire_of\((\w+)\)\)')
    # Pattern 2: m.output(f"{prefix}_NAME", EXPR) where EXPR doesn't use wire_of
    pat_raw = re.compile(r'^(\s+)m\.output\(f"\{prefix\}_(\w+)",\s*(\w+)\)')

    for line in lines:
        new_lines.append(line)

        m1 = pat_wire.match(line)
        if m1:
            indent = m1.group(1)
            name = m1.group(2)
            var = m1.group(3)
            new_lines.append(f'{indent}_out["{name}"] = {var}')
            count += 1
            continue

        m2 = pat_raw.match(line)
        if m2 and not pat_wire.match(line):
            indent = m2.group(1)
            name = m2.group(2)
            raw_var = m2.group(3)
            # Wrap raw wire in cas at current cycle
            new_lines.append(
                f'{indent}_out["{name}"] = cas(domain, {raw_var}, cycle=domain.cycle_index)'
            )
            count += 1

    return "\n".join(new_lines), count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--file", type=str, default=None)
    args = parser.parse_args()

    total = {"sig": 0, "init": 0, "input": 0, "output": 0, "ret": 0}

    if args.file:
        modules = [(f, fn) for f, fn in ALL_MODULES if f == args.file]
        if not modules:
            return 1
    else:
        modules = ALL_MODULES

    processed = 0
    for rel_path, func_name in modules:
        fp = XS_ROOT / rel_path
        if not fp.exists():
            continue
        s = transform_file(fp, func_name, dry_run=args.dry_run)
        for k in total:
            total[k] += s[k]
        processed += 1

    for k, _v in total.items():
        pass
    if args.dry_run:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
