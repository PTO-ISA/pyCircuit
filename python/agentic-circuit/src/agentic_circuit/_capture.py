"""Shared source-closure capture used by the ACC Python driver."""

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

from ._canonical_json import canonical_json_bytes
from ._capture_worker import CaptureWorkerRequest, CaptureWorkerResult, run_capture_worker
from ._diagnostics import Diagnostic
from ._workspace import UserInputError, WorkspaceConfig


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
    source_modules = tuple(getattr(arguments, "source_modules", ()))
    with tempfile.TemporaryDirectory(prefix="agentic-capture-") as temporary:
        return run_capture_worker(
            CaptureWorkerRequest(
                python=sys.executable,
                workspace=workspace.root,
                entry=_entry(arguments, workspace),
                system=(
                    source_modules[0]
                    if source_modules
                    else getattr(arguments, "system", None)
                    or workspace.default_system
                ),
                static_arguments=tuple(getattr(arguments, "static_arguments", ())),
                component_roots=workspace.component_roots,
                private_output=Path(temporary) / "capture",
                timeout=float(getattr(arguments, "timeout", 30.0)),
                source_closure=True,
                entry_kind="source_unit" if source_modules else "system",
                source_modules=source_modules,
            )
        )


def has_errors(diagnostics: tuple[Diagnostic, ...]) -> bool:
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
