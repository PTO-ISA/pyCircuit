"""Read-only deterministic architecture inspection command."""

from __future__ import annotations

from typing import NoReturn

from .._diagnostics import Diagnostic
from .._exit_codes import ExitCode
from .._inspect import (
    InspectionError,
    InspectionRequest,
    inspect_model,
    render_dot,
    render_text,
)
from .._output import OutputSink
from .._workspace import UserInputError, WorkspaceConfig
from .check import _has_errors, capture


def _fail(message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(
            stage="inspect",
            code="ACPY-INSPECT-001",
            severity="error",
            message=message,
        )
    )


def run(arguments: object, workspace: WorkspaceConfig, sink: OutputSink) -> int:
    kind = getattr(arguments, "view")
    path = getattr(arguments, "path", None)
    selected_format = getattr(arguments, "format", None)
    if selected_format == "dot" and kind != "graph":
        _fail("--format dot requires inspect graph")
    if selected_format in {"dot", "text"} and sink.format != "text":
        _fail(f"--format {selected_format} cannot be combined with structured output")
    if selected_format == "json":
        sink.format = "json"
    system = getattr(arguments, "system", None) or workspace.default_system
    request = InspectionRequest(
        kind=kind,
        system=system,
        path=path,
        format=selected_format or ("json" if sink.format != "text" else "text"),
    )
    frontend = capture(arguments, workspace)
    if _has_errors(frontend.diagnostics):
        sink.diagnostics(frontend.diagnostics)
        return ExitCode.USER_INPUT
    if frontend.acpy is None or frontend.acir is None:
        _fail("frontend produced incomplete inspection artifacts")
    try:
        result = inspect_model(frontend.acpy, frontend.acir, request)
    except InspectionError as error:
        _fail(str(error))
    if selected_format == "dot":
        sink.stdout.write(render_dot(result))
    else:
        sink.result(result.to_json(), human=render_text(result))
    return ExitCode.SUCCESS
