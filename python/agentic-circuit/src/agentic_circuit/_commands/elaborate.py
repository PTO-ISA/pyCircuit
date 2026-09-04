"""Deterministic frontend artifact publication."""

from __future__ import annotations

from pathlib import Path

from .._canonical_json import sha256_bytes
from .._diagnostics import Diagnostic
from .._native_api import NativeRequest, run_native_compiler
from .._output import OutputSink
from .._staging import ArtifactStage
from .._workspace import UserInputError, WorkspaceConfig
from .check import _has_errors, capture


def _output_path(arguments: object) -> Path:
    value = getattr(arguments, "output", None)
    if value is None:
        raise UserInputError(
            Diagnostic(
                stage="elaborate",
                code="ACPY-CLI-OUTPUT",
                severity="error",
                message="elaborate requires -o/--output",
            )
        )
    path = Path(value)
    return path.resolve() if path.is_absolute() else (Path.cwd() / path).resolve()


def run(arguments: object, workspace: WorkspaceConfig, sink: OutputSink) -> int:
    frontend = capture(arguments, workspace)
    if _has_errors(frontend.diagnostics):
        sink.diagnostics(frontend.diagnostics)
        return 2
    emit = getattr(arguments, "emit")
    data = frontend.acpy if emit == "acpy" else frontend.acir
    if data is None:
        sink.diagnostics(
            (
                Diagnostic(
                    stage="elaborate",
                    code="ACPY-VERIFY-001",
                    severity="error",
                    message=f"frontend produced no {emit} artifact",
                ),
            )
        )
        return 2
    if emit == "acir":
        from .build import _binding_registry

        native = run_native_compiler(
            NativeRequest(
                acir=data,
                stop_after="acir-verify",
                emits=(),
                options=(
                    ("binding_registry", _binding_registry(workspace.component_roots)),
                ),
            )
        )
        if _has_errors(native.diagnostics):
            sink.diagnostics(native.diagnostics)
            return 2
    output = _output_path(arguments)
    relative = output.name
    with ArtifactStage(output.parent, expected=(relative,)) as stage:
        stage.write_bytes(relative, data)
        stage.commit(allow_replace=(relative,) if output.exists() else ())
    fingerprint = sha256_bytes(data)
    sink.result(
        {
            "schema": "agentic-circuit-elaborate-result",
            "version": "0.1",
            "contract_epoch": "0.5",
            "emit": emit,
            "path": output.as_posix(),
            "sha256": fingerprint,
        },
        human=f"wrote {output}",
    )
    return 0
