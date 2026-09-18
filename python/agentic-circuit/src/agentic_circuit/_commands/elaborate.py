"""Deterministic frontend artifact publication."""

from __future__ import annotations

from pathlib import Path

from .._diagnostics import Diagnostic
from .._exit_codes import ExitCode
from .._native_api import NativeRequest, run_native_compiler
from .._output import OutputSink
from .._staging import ArtifactStage
from .._workspace import UserInputError, WorkspaceConfig
from .check import _has_errors, binding_registry, capture


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
        return ExitCode.USER_INPUT
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
        return ExitCode.USER_INPUT
    if emit == "acir":
        native = run_native_compiler(
            NativeRequest(
                acir=data,
                stop_after="acir-verify",
                emits=(),
                options=(
                    ("binding_registry", binding_registry(workspace.component_roots)),
                ),
            )
        )
        if _has_errors(native.diagnostics):
            sink.diagnostics(native.diagnostics)
            return ExitCode.USER_INPUT
    output = _output_path(arguments)
    relative = output.name
    with ArtifactStage(output.parent, expected=(relative,)) as stage:
        stage.write_bytes(relative, data)
        stage.commit(allow_replace=(relative,) if output.exists() else ())
    sink.result(
        {
            "schema": "agentic-circuit-elaborate-result",
            "version": "0.1",
            "emit": emit,
            "path": output.as_posix(),
        },
        human=f"wrote {output}",
    )
    return ExitCode.SUCCESS
