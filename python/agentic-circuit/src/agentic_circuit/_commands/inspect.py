"""Read-only deterministic architecture inspection command."""

from __future__ import annotations

import json
import re
from pathlib import Path, PurePosixPath
from typing import NoReturn

from .._canonical_json import canonical_json_bytes, sha256_bytes
from .._diagnostics import Diagnostic
from .._inspect import (
    InspectionError,
    InspectionRequest,
    inspect_build,
    inspect_model,
    render_dot,
    render_text,
)
from .._output import OutputSink
from .._workspace import UserInputError, WorkspaceConfig
from .check import _has_errors, capture


_BUILD_MANIFEST_KEYS = {
    "schema",
    "version",
    "contract_epoch",
    "project",
    "system",
    "source_files",
    "normalized_acir_sha256",
    "compiler",
    "pass_pipeline",
    "providers",
    "component_specializations",
    "protocol_identities",
    "artifacts",
    "validation_gates",
    "build_profile",
    "instrumentation_layers",
    "specialization_inputs",
    "build_fingerprint",
}
_FINGERPRINT = re.compile(r"^sha256:[0-9a-f]{64}$")


def _fail(message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(
            stage="inspect",
            code="ACPY-INSPECT-001",
            severity="error",
            message=message,
        )
    )


def _relative_path(value: object) -> PurePosixPath:
    if type(value) is not str:
        _fail("current build pointer path is invalid")
    path = PurePosixPath(value)
    if (
        path.is_absolute()
        or not path.parts
        or any(part in ("", ".", "..") for part in path.parts)
        or "\\" in value
    ):
        _fail("current build pointer path is invalid")
    return path


def _read_json(path: Path, label: str) -> dict[str, object]:
    try:
        if path.is_symlink() or not path.is_file():
            _fail(f"{label} is not a regular file")
        data = path.read_bytes()
        value = json.loads(data.decode("utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        _fail(f"{label} is invalid: {error}")
    if type(value) is not dict:
        _fail(f"{label} must be a JSON object")
    try:
        if canonical_json_bytes(value) != data:
            _fail(f"{label} is not canonical")
    except ValueError as error:
        _fail(f"{label} is not canonical: {error}")
    return value


def _current_build(workspace: WorkspaceConfig, system: str) -> dict[str, object] | None:
    for root in (workspace.build_root / system, workspace.build_root):
        pointer_path = root / "current.json"
        if not pointer_path.exists():
            continue
        pointer = _read_json(pointer_path, "current build pointer")
        if set(pointer) != {"build_fingerprint", "path"}:
            _fail("current build pointer has unknown fields")
        fingerprint = pointer["build_fingerprint"]
        if type(fingerprint) is not str or not _FINGERPRINT.fullmatch(fingerprint):
            _fail("current build pointer fingerprint is invalid")
        relative = _relative_path(pointer["path"])
        if relative != PurePosixPath("builds") / fingerprint:
            _fail("current build pointer path does not match its fingerprint")
        manifest_path = root.joinpath(*relative.parts, "build-manifest.json")
        if not manifest_path.resolve().is_relative_to(root.resolve()):
            _fail("current build pointer escapes its build root")
        manifest = _read_json(manifest_path, "current build manifest")
        if (
            set(manifest) != _BUILD_MANIFEST_KEYS
            or manifest.get("schema") != "agentic-circuit-build-manifest"
            or manifest.get("version") != "0.1"
            or manifest.get("contract_epoch") != "0.4"
            or manifest.get("build_fingerprint") != pointer["build_fingerprint"]
        ):
            _fail("current build manifest identity does not match its pointer")
        artifacts = manifest.get("artifacts")
        if type(artifacts) is not list:
            _fail("current build manifest artifacts are invalid")
        for artifact in artifacts:
            if type(artifact) is not dict or set(artifact) != {
                "path",
                "kind",
                "sha256",
            }:
                _fail("current build manifest artifact identity is invalid")
            artifact_path = _relative_path(artifact["path"])
            artifact_hash = artifact["sha256"]
            if type(artifact_hash) is not str or not _FINGERPRINT.fullmatch(
                artifact_hash
            ):
                _fail("current build manifest artifact hash is invalid")
            resolved = manifest_path.parent.joinpath(*artifact_path.parts)
            try:
                if (
                    not resolved.resolve().is_relative_to(
                        manifest_path.parent.resolve()
                    )
                    or resolved.is_symlink()
                    or not resolved.is_file()
                ):
                    _fail("current build artifact is not a regular file")
                if sha256_bytes(resolved.read_bytes()) != artifact_hash:
                    _fail("current build artifact hash does not match")
            except OSError as error:
                _fail(f"cannot verify current build artifact: {error}")
        return manifest
    return None


def run(arguments: object, workspace: WorkspaceConfig, sink: OutputSink) -> int:
    kind = getattr(arguments, "view")
    path = getattr(arguments, "path", None)
    selected_format = getattr(arguments, "format", None)
    if kind == "artifacts" and path is not None:
        _fail("artifact inspection does not accept --path")
    if selected_format == "dot" and kind != "graph":
        _fail("--format dot requires inspect graph")
    if selected_format == "dot" and sink.format != "text":
        _fail("--format dot cannot be combined with structured output")
    if selected_format == "text" and sink.format != "text":
        _fail("--format text cannot be combined with structured output")
    if selected_format == "json":
        sink.format = "json"
    system = getattr(arguments, "system", None) or workspace.default_system
    try:
        request = InspectionRequest(
            kind=kind,
            system=system,
            path=path,
            format=selected_format or ("json" if sink.format != "text" else "text"),
        )
        build_manifest = (
            _current_build(workspace, system) if kind == "artifacts" else None
        )
        if build_manifest is not None:
            result = inspect_build(build_manifest, request)
        else:
            frontend = capture(arguments, workspace)
            if _has_errors(frontend.diagnostics):
                sink.diagnostics(frontend.diagnostics)
                return 2
            if frontend.acpy is None or frontend.acir is None:
                _fail("frontend produced incomplete inspection artifacts")
            result = inspect_model(frontend.acpy, frontend.acir, request)
    except InspectionError as error:
        _fail(str(error))
    if selected_format == "dot":
        sink.stdout.write(render_dot(result))
    else:
        sink.result(result.to_json(), human=render_text(result))
    return 0
