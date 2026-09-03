#!/usr/bin/env python3
"""Audit per-operation valid+invalid test coverage for ACIR and ACSim dialects.

Reads .td definitions and scans lit test files to report which operations
have valid (positive) and invalid (negative) test coverage.
Exits non-zero if any public op lacks both valid and invalid tests.
"""

import re
import sys
from pathlib import Path
from collections import defaultdict

REPO = Path(__file__).resolve().parents[2]


def extract_ops(td_path: Path) -> dict[str, str]:
    """Extract {CppName: assemblyName} from a .td file."""
    ops = {}
    text = td_path.read_text()
    # Match: def ACIR_FooOp : ACIR_Op<"ac.foo", ...>
    for m in re.finditer(
        r'def\s+(ACIR_\w+Op|ACSim_\w+Op)\s*:\s*\w+_Op<"([^"]+)"', text
    ):
        ops[m.group(1)] = m.group(2)
    return ops


def find_op_in_file(op_asm: str, filepath: Path) -> bool:
    """Check if the assembly name appears in the file."""
    try:
        return op_asm in filepath.read_text()
    except Exception:
        return False


def main():
    acir_ops = extract_ops(REPO / "compiler/acir/include/acir/Dialect/ACIR/ACIROps.td")
    acsim_ops = extract_ops(
        REPO / "compiler/acir/include/acir/Dialect/ACSim/ACSimOps.td"
    )
    all_ops = {**acir_ops, **acsim_ops}

    # Find test files
    test_dir = REPO / "tests/mlir/agentic-circuit"
    valid_files = [f for f in test_dir.rglob("*.mlir") if "invalid" not in f.name]
    invalid_files = [f for f in test_dir.rglob("*.mlir") if "invalid" in f.name]

    coverage = defaultdict(lambda: {"valid": False, "invalid": False})

    for cpp_name, asm_name in all_ops.items():
        for f in valid_files:
            if find_op_in_file(asm_name, f):
                coverage[cpp_name]["valid"] = True
                break
        for f in invalid_files:
            if find_op_in_file(asm_name, f):
                coverage[cpp_name]["invalid"] = True
                break

    missing_valid = []
    missing_invalid = []
    missing_both = []

    for cpp_name in sorted(all_ops):
        v = coverage[cpp_name]["valid"]
        i = coverage[cpp_name]["invalid"]
        if not v and not i:
            missing_both.append(cpp_name)
        elif not v:
            missing_valid.append(cpp_name)
        elif not i:
            missing_invalid.append(cpp_name)

    total = len(all_ops)
    covered_both = total - len(missing_valid) - len(missing_invalid) - len(missing_both)
    covered_valid = total - len(missing_valid) - len(missing_both)
    covered_invalid = total - len(missing_invalid) - len(missing_both)

    print(f"=== ACIR/ACSim Op Test Coverage Audit ===")
    print(f"Total ops: {total}")
    print(f"  Both valid+invalid: {covered_both} ({100*covered_both//total}%)")
    print(f"  Valid only: {len(missing_invalid)}")
    print(f"  Invalid only: {len(missing_valid)}")
    print(f"  Missing both: {len(missing_both)}")
    print()

    if missing_both:
        print(f"--- Missing both valid and invalid tests ({len(missing_both)}) ---")
        for name in missing_both:
            print(f"  {name} ({all_ops[name]})")
        print()

    if missing_valid:
        print(f"--- Missing valid tests ({len(missing_valid)}) ---")
        for name in missing_valid:
            print(f"  {name} ({all_ops[name]})")
        print()

    if missing_invalid:
        print(f"--- Missing invalid tests ({len(missing_invalid)}) ---")
        for name in missing_invalid:
            print(f"  {name} ({all_ops[name]})")
        print()

    missing_any = len(missing_both) + len(missing_valid) + len(missing_invalid)
    if missing_any == 0:
        print("PASS: Every public operation has both valid and invalid test coverage.")
    else:
        print(
            f"FAIL: {missing_any} operations have incomplete coverage "
            f"({len(missing_both)} missing-both, "
            f"{len(missing_valid)} missing-valid, "
            f"{len(missing_invalid)} missing-invalid)."
        )

    sys.exit(0 if missing_any == 0 else 1)


if __name__ == "__main__":
    main()
