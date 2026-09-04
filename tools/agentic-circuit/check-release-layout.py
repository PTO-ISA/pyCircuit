#!/usr/bin/env python3
"""Validate the integrated repository layout and release-neutral AC paths."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


FORBIDDEN = re.compile(
    r"(?:^|[-_./])(?:v0?\d+(?:[._]\d+)*|phase[-_]?\d+[a-z]?)" r"(?:[-_./]|$)",
    re.IGNORECASE,
)
REQUIRED_ROOTS = (
    Path("docs/acir/spec"),
    Path("examples/agentic-circuit/pipelines"),
    Path("examples/agentic-circuit/memory"),
    Path("examples/agentic-circuit/blocks"),
    Path("examples/agentic-circuit/architecture"),
    Path("examples/agentic-circuit/workspaces"),
    Path("third_party/references"),
    Path("tests/goldens/agentic-circuit"),
)
LAYOUT_REQUIRED_ROOTS = (
    Path("python/pycircuit/src/pycircuit"),
    Path("python/agentic-circuit/src/agentic_circuit"),
    Path("compiler/mlir"),
    Path("compiler/acir"),
    Path("library/cpp"),
    Path("library/verilog"),
    Path("simulator/gfsim"),
    Path("schemas/agentic-circuit"),
    Path("examples/pycircuit"),
    Path("examples/agentic-circuit"),
    Path("designs/blocks"),
    Path("third_party/references/davincioo-gfsim"),
    Path("toolchains/agentic-circuit"),
    Path("tools/agentic-circuit"),
)
ACTIVE_ROOTS = REQUIRED_ROOTS + (
    Path("compiler/acir"),
    Path("python/agentic-circuit"),
    Path("schemas/agentic-circuit"),
    Path("simulator/gfsim"),
    Path("tests/cpp/agentic-circuit"),
    Path("tests/integration/agentic-circuit"),
    Path("tests/mlir/agentic-circuit"),
    Path("tests/python/agentic-circuit"),
    Path("toolchains/agentic-circuit"),
    Path("tools/agentic-circuit"),
)
DEPRECATED_ROOTS = (
    Path("components"),
    Path("runtime"),
    Path("designs/examples"),
    Path("designs/BypassUnit"),
    Path("designs/IssueQueue"),
    Path("designs/RegisterFile"),
    Path("designs/XiangShan-pyc"),
    Path("contrib/linx"),
    Path("boards"),
    Path("janus"),
    Path("integrations"),
    Path("platforms"),
    Path("examples/pycircuit/linxcore_frontend_pipeline"),
)
DEPRECATED_TEXT_PATTERNS = {
    "components/agentic-circuit": re.compile(
        r"(?:components/agentic-circuit|[\"']components[\"']\s*/\s*[\"']agentic-circuit[\"'])"
    ),
    "runtime/cpp-or-verilog": re.compile(
        r"(?:runtime/(?:cpp|verilog)|[\"']runtime[\"']\s*/\s*[\"'](?:cpp|verilog)[\"'])"
    ),
    "compiler/frontend": re.compile(
        r"(?:compiler/frontend|[\"']compiler[\"']\s*/\s*[\"']frontend[\"'])"
    ),
    "designs/examples": re.compile(
        r"(?:designs/examples|[\"']designs[\"']\s*/\s*[\"']examples[\"'])"
    ),
    "contrib/linx": re.compile(
        r"(?:contrib/linx|[\"']contrib[\"']\s*/\s*[\"']linx[\"'])"
    ),
    "boards/zybo_z7_20": re.compile(
        r"(?:boards/zybo_z7_20|[\"']boards[\"']\s*/\s*[\"']zybo_z7_20[\"'])"
    ),
    "include/pyc support trees": re.compile(
        r"(?:include/pyc/(?:cpp|verilog)|[\"']include[\"']\s*/\s*[\"']pyc[\"']\s*/\s*[\"'](?:cpp|verilog)[\"'])"
    ),
}
TEXT_SCAN_EXCLUDES = {
    "tools/agentic-circuit/check-release-layout.py",
    "tests/python/agentic-circuit/contracts/test_contracts.py",
}
TEXT_SUFFIXES = {".md", ".py", ".sh", ".toml", ".yaml", ".yml"}


def tracked_paths(root: Path) -> list[str]:
    completed = subprocess.run(
        ("git", "ls-files", "--", *(path.as_posix() for path in ACTIVE_ROOTS)),
        cwd=root,
        text=True,
        capture_output=True,
        check=True,
    )
    return completed.stdout.splitlines()


def tracked_deprecated_paths(root: Path) -> list[str]:
    completed = subprocess.run(
        ("git", "ls-files", "--", *(path.as_posix() for path in DEPRECATED_ROOTS)),
        cwd=root,
        text=True,
        capture_output=True,
        check=True,
    )
    return completed.stdout.splitlines()


def tracked_text_files(root: Path) -> list[Path]:
    completed = subprocess.run(
        ("git", "ls-files", "-z"),
        cwd=root,
        capture_output=True,
        check=True,
    )
    files = []
    for raw_path in completed.stdout.split(b"\0"):
        if not raw_path:
            continue
        relative = raw_path.decode()
        if relative in TEXT_SCAN_EXCLUDES or relative.startswith("docs/gates/logs/"):
            continue
        path = root / relative
        if path.is_file() and path.suffix.lower() in TEXT_SUFFIXES:
            files.append(path)
    return files


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    errors = [
        f"forbidden release/phase token in tracked path: {file_path}"
        for file_path in tracked_paths(root)
        if FORBIDDEN.search(file_path)
    ]
    errors.extend(
        f"deprecated repository root is still tracked: {file_path}"
        for file_path in tracked_deprecated_paths(root)
    )
    for path in tracked_text_files(root):
        text = path.read_text(encoding="utf-8")
        for label, pattern in DEPRECATED_TEXT_PATTERNS.items():
            if pattern.search(text):
                errors.append(
                    f"deprecated path reference {label} in {path.relative_to(root)}"
                )
    errors.extend(
        f"deprecated repository root still exists: {deprecated}"
        for deprecated in DEPRECATED_ROOTS
        if (root / deprecated).exists()
    )
    errors.extend(
        f"required repository root is missing: {required}"
        for required in REQUIRED_ROOTS + LAYOUT_REQUIRED_ROOTS
        if not (root / required).is_dir()
    )
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("release layout: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
