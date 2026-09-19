"""Fast trusted frontend validation through native ACIR verification."""

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

from .._canonical_json import canonical_json_bytes
from .._capture_worker import (
    CaptureWorkerRequest,
    CaptureWorkerResult,
    run_capture_worker,
)
from .._diagnostics import Diagnostic
from .._exit_codes import ExitCode
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
    selected_module = getattr(arguments, "module", None)
    source_specializations = tuple(
        getattr(arguments, "source_specializations", ())
    )
    entry_kind = (
        "source_unit"
        if source_specializations
        else "module"
        if selected_module
        else "system"
    )
    with tempfile.TemporaryDirectory(prefix="agentic-capture-") as temporary:
        return run_capture_worker(
            CaptureWorkerRequest(
                python=sys.executable,
                workspace=workspace.root,
                entry=_entry(arguments, workspace),
                system=(
                    selected_module
                    or (
                        source_specializations[0][0]
                        if source_specializations
                        else None
                    )
                    or getattr(arguments, "system", None)
                    or workspace.default_system
                ),
                static_arguments=tuple(
                    getattr(arguments, "static_arguments", ())
                ),
                component_roots=workspace.component_roots,
                private_output=Path(temporary) / "capture",
                timeout=float(getattr(arguments, "timeout", 30.0)),
                jit_source_closure=bool(
                    getattr(arguments, "jit_source_closure", False)
                ),
                entry_kind=entry_kind,
                source_specializations=source_specializations,
            )
        )


def _has_errors(diagnostics: tuple[Diagnostic, ...]) -> bool:
    return any(item.severity == "error" for item in diagnostics)


def binding_registry(component_roots: tuple[Path, ...]) -> bytes:
    candidates: list[object] = []
    requests: list[object] = []
    for root in sorted(component_roots):
        for path in sorted(root.rglob("*.binding.json")):
            try:
                document = json.loads(path.read_text(encoding="utf-8"))
            except (OSError, UnicodeError, json.JSONDecodeError) as error:
                raise UserInputError(
                    Diagnostic(
                        stage="acir-core",
                        code="ACLOWER-BINDING-OPTIONS",
                        severity="error",
                        message=f"cannot load {path}: {error}",
                    )
                ) from error
            if type(document) is not dict or set(document) != {
                "candidates",
                "requests",
            }:
                raise UserInputError(
                    Diagnostic(
                        stage="acir-core",
                        code="ACLOWER-BINDING-OPTIONS",
                        severity="error",
                        message=f"binding registry {path} is not closed",
                    )
                )
            if not isinstance(document["candidates"], list) or not isinstance(
                document["requests"], list
            ):
                raise UserInputError(
                    Diagnostic(
                        stage="acir-core",
                        code="ACLOWER-BINDING-OPTIONS",
                        severity="error",
                        message=f"binding registry {path} arrays are invalid",
                    )
                )
            candidates.extend(document["candidates"])
            requests.extend(document["requests"])
    return canonical_json_bytes({"candidates": candidates, "requests": requests})


def run(arguments: object, workspace: WorkspaceConfig, sink: OutputSink) -> int:
    frontend = capture(arguments, workspace)
    if _has_errors(frontend.diagnostics):
        sink.diagnostics(frontend.diagnostics)
        return ExitCode.USER_INPUT
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
            return ExitCode.USER_INPUT
        native = run_native_compiler(
            NativeRequest(
                acir=frontend.acir,
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
        stage = "acir-core"
    sink.result(
        {
            "schema": "agentic-circuit-check-result",
            "version": "0.1",
            "project": workspace.project_name,
            "system": getattr(arguments, "system", None) or workspace.default_system,
            "frontend": frontend.frontend_kind,
            "stage": stage,
            "status": "passed",
        },
        human=f"check passed through {stage}",
    )
    return ExitCode.SUCCESS
