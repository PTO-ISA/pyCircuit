from __future__ import annotations

import os
from pathlib import Path


def bundled_toolchain_root() -> Path | None:
    root = Path(__file__).resolve().parent / "_toolchain"
    if root.is_dir():
        return root
    return None


def _installed_prefix_root() -> Path | None:
    package = Path(__file__).resolve().parent
    python_root = package.parent
    share_package_root = python_root.parent
    share_root = share_package_root.parent
    if (
        package.name == "pycircuit"
        and python_root.name == "python"
        and share_package_root.name == "pycircuit"
        and share_root.name == "share"
    ):
        prefix = share_root.parent
        if (prefix / "bin").is_dir():
            return prefix
    return None


def tool_executable(name: str) -> Path | None:
    if not name or Path(name).name != name:
        return None

    suffixes = [""]
    if os.name == "nt":
        suffixes.insert(0, ".exe")

    roots = (_installed_prefix_root(), bundled_toolchain_root())
    checked: set[Path] = set()
    for root in roots:
        if root is None or root in checked:
            continue
        checked.add(root)
        for suffix in suffixes:
            candidate = root / "bin" / f"{name}{suffix}"
            if candidate.is_file() and os.access(candidate, os.X_OK):
                return candidate
    return None
