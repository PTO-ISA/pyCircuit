"""Filesystem-backed access to installed package resources."""

from __future__ import annotations

from importlib.resources import files
from pathlib import Path


def repository_root() -> Path:
    """Locate a source checkout without assuming a fixed package depth."""

    for parent in Path(__file__).resolve().parents:
        if (
            (parent / "schemas" / "agentic-circuit").is_dir()
            and (parent / "python" / "agentic-circuit").is_dir()
            and (parent / "CMakeLists.txt").is_file()
        ):
            return parent
    raise FileNotFoundError("Agentic Circuit source repository is unavailable")


def resource_directory(name: str) -> Path:
    try:
        resource = files("agentic_circuit._data").joinpath(name)
        installed: Path | None = Path(str(resource))
    except ModuleNotFoundError:
        installed = None
    if installed is not None and installed.is_dir():
        return installed
    source = repository_root() / name
    if source.is_dir():
        return source
    raise FileNotFoundError(f"Agentic Circuit {name} resources are unavailable")
