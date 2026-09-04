"""Fast trusted frontend validation through native ACIR verification."""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

from .._capture_worker import (
    CaptureWorkerRequest,
    CaptureWorkerResult,
    run_capture_worker,
)
from .._diagnostics import Diagnostic
from .._native_api import NativeRequest, run_native_compiler
from .._output import OutputSink
from .._workspace import UserInputError, WorkspaceConfig


def _entry(arguments: object, workspace: WorkspaceConfig) -> Path:
    value = getattr(arguments, "architecture", None)
    entry = (workspace.root / value).resolve() if value else workspace.architecture
    if not entry.is_relative_to(workspace.root):
        raise UserInputError(
            Diagnostic(
                stage="frontend-capture",
                code="ACPY-CONFIG-003",
                severity="error",
                message="architecture entry escapes the workspace",
            )
        )
    return entry


def capture(arguments: object, workspace: WorkspaceConfig) -> CaptureWorkerResult:
    with tempfile.TemporaryDirectory(prefix="agentic-capture-") as temporary:
        return run_capture_worker(
            CaptureWorkerRequest(
                python=sys.executable,
                workspace=workspace.root,
                entry=_entry(arguments, workspace),
                system=getattr(arguments, "system", None) or workspace.default_system,
                static_arguments=(),
                component_roots=workspace.component_roots,
                private_output=Path(temporary) / "capture",
            )
        )


def _has_errors(diagnostics: tuple[Diagnostic, ...]) -> bool:
    return any(item.severity == "error" for item in diagnostics)


def run(arguments: object, workspace: WorkspaceConfig, sink: OutputSink) -> int:
    frontend = capture(arguments, workspace)
    if _has_errors(frontend.diagnostics):
        sink.diagnostics(frontend.diagnostics)
        return 2
    stage = "acpy-verify"
    if getattr(arguments, "stop_after", None) != "acpy-verify":
        if frontend.acir is None:
            sink.diagnostics(
                (
                    Diagnostic(
                        stage="acir-elaboration",
                        code="ACPY-VERIFY-001",
                        severity="error",
                        message="frontend produced no ACIR artifact",
                    ),
                )
            )
            return 2
        from .build import _binding_registry

        native = run_native_compiler(
            NativeRequest(
                acir=frontend.acir,
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
        stage = "acir-core"
    sink.result(
        {
            "schema": "agentic-circuit-check-result",
            "version": "0.1",
            "contract_epoch": "0.5",
            "project": workspace.project_name,
            "system": getattr(arguments, "system", None) or workspace.default_system,
            "frontend": frontend.frontend_kind,
            "stage": stage,
            "status": "passed",
        },
        human=f"check passed through {stage}",
    )
    return 0
