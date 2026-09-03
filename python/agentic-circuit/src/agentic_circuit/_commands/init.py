"""Specification-only workspace initialization."""

from __future__ import annotations

from pathlib import Path, PurePosixPath

from .._diagnostics import Diagnostic
from .._output import OutputSink
from .._capabilities import schema_root
from .._staging import ArtifactStage
from .._workspace import UserInputError

_FILE_NAMES = ("agentic-circuit.toml", "architecture.py")


def _files() -> dict[str, str]:
    templates = schema_root() / "templates"
    return {
        name: templates.joinpath(name).read_text(encoding="utf-8")
        for name in _FILE_NAMES
    }


def _error(code: str, message: str) -> UserInputError:
    return UserInputError(
        Diagnostic(stage="init", code=code, severity="error", message=message)
    )


def run(arguments: object, sink: OutputSink) -> int:
    files = _files()
    destination = Path(getattr(arguments, "directory", ".")).resolve()
    force_values = tuple(getattr(arguments, "force", ()) or ())
    force = {PurePosixPath(item).as_posix() for item in force_values}
    unknown_force = sorted(force - set(files))
    if unknown_force:
        raise _error("ACPY-INIT-002", f"unknown init target: {unknown_force[0]}")
    conflicts = sorted(
        name
        for name in files
        if destination.joinpath(name).exists() and name not in force
    )
    if conflicts:
        raise _error(
            "ACPY-INIT-001",
            f"init target already exists and is not forced: {conflicts[0]}",
        )

    result = {
        "schema": "agentic-circuit-init-result",
        "version": "0.1",
        "contract_epoch": "0.4",
        "directory": destination.as_posix(),
        "files": sorted(files),
        "dry_run": bool(getattr(arguments, "dry_run", False)),
    }
    if result["dry_run"]:
        sink.result(result, human=f"Would initialize {destination}")
        return 0

    with ArtifactStage(destination, expected=files) as stage:
        for name, contents in sorted(files.items()):
            stage.write_text(name, contents)
        stage.commit(allow_replace=force)
    sink.result(result, human=f"Initialized {destination}")
    return 0
