from __future__ import annotations

import os
import sys
from pathlib import Path


def bundled_toolchain_root() -> Path | None:
    root = Path(__file__).resolve().parent / "_toolchain"
    if root.is_dir():
        return root
    return None


def tool_executable(name: str) -> Path | None:
    root = bundled_toolchain_root()
    if root is None:
        return None

    suffixes = [""]
    if os.name == "nt":
        suffixes.insert(0, ".exe")

    for suffix in suffixes:
        candidate = root / "bin" / f"{name}{suffix}"
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate
    return None


def _exec_tool(name: str, argv: list[str]) -> int:
    exe = tool_executable(name)
    if exe is None:
        root = bundled_toolchain_root()
        root_text = str(root) if root is not None else "<missing>"
        raise SystemExit(f"bundled tool `{name}` not found under: {root_text}")

    env = os.environ.copy()
    env.setdefault("PYC_TOOLCHAIN_ROOT", str(exe.parent.parent))
    os.execvpe(str(exe), [str(exe), *argv], env)
    return 0


def main() -> int:
    return _exec_tool("pycc", sys.argv[1:])


def pyc_opt_main() -> int:
    return _exec_tool("pyc-opt", sys.argv[1:])
