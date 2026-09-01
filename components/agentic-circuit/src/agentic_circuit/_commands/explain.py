"""Stable diagnostic explanation lookup."""

from __future__ import annotations

from .._capabilities import diagnostic_catalog
from .._diagnostics import Diagnostic
from .._output import OutputSink
from .._workspace import UserInputError


def run(arguments: object, sink: OutputSink) -> int:
    code = getattr(arguments, "code")
    entries = diagnostic_catalog().get("entries")
    if type(entries) is not list:
        raise ValueError("packaged diagnostic catalog is invalid")
    matches = [
        item
        for item in entries
        if type(item) is dict and item.get("code") == code
    ]
    if len(matches) != 1:
        raise UserInputError(
            Diagnostic(
                stage="explain",
                code="ACPY-SCHEMA-001",
                severity="error",
                message=f"diagnostic code is unknown: {code}",
            )
        )
    document = {
        "schema": "agentic-circuit-diagnostic-explanation",
        "version": "0.1",
        "contract_epoch": "0.4",
        **matches[0],
    }
    sink.result(document, human=f"{code}: {matches[0]['rule']}")
    return 0
