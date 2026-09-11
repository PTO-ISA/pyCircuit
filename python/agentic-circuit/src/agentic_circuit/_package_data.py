"""Filesystem-backed access to installed package resources."""

from __future__ import annotations

from importlib.resources import files
from pathlib import Path


def resource_directory(name: str) -> Path:
    try:
        resource = files("agentic_circuit._data").joinpath(name)
        installed: Path | None = Path(str(resource))
    except ModuleNotFoundError:
        installed = None
    if installed is not None and installed.is_dir():
        return installed
    repository = Path(__file__).resolve().parents[4]
    source = repository / name
    if source.is_dir():
        return source
    raise FileNotFoundError(f"Agentic Circuit {name} resources are unavailable")
