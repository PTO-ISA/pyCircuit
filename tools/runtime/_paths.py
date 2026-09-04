"""Path helpers shared by the runtime promotion scripts.

The promotion scripts originated in the standalone Agentic Circuit checkout,
where runtime files were addressed as ``verilog/...`` below a different root.
This module keeps the migration boundary in one place: the checked-in runtime
root is ``<repo>/library/verilog`` and catalog paths are relative to it.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any


def repo_root(anchor: str | Path) -> Path:
    """Return the pyCircuit repository root for a file in ``tools/runtime``."""

    return Path(anchor).resolve().parents[2]


def runtime_root(anchor: str | Path) -> Path:
    return repo_root(anchor) / "library" / "verilog"


def normalize_runtime_path(value: str) -> str:
    """Convert legacy ``verilog/`` and ``build/`` paths to repo conventions."""

    value = value.replace("\\", "/")
    if value.startswith("verilog/"):
        value = value[len("verilog/") :]
    if value.startswith("build/"):
        value = ".pycircuit_out/" + value[len("build/") :]
    return value


def normalize_spec(value: Any) -> Any:
    """Recursively normalize path-like strings in a promotion spec."""

    if isinstance(value, str):
        return normalize_runtime_path(value)
    if isinstance(value, list):
        return [normalize_spec(item) for item in value]
    if isinstance(value, tuple):
        return tuple(normalize_spec(item) for item in value)
    if isinstance(value, dict):
        return {key: normalize_spec(item) for key, item in value.items()}
    return value


def normalize_specs(value: Any) -> Any:
    return normalize_spec(value)
