"""Stable diagnostic explanation lookup."""

from __future__ import annotations

from typing import Any

from .._capabilities import diagnostic_catalog
from .._diagnostics import Diagnostic
from .._exit_codes import ExitCode
from .._output import OutputSink
from .._workspace import UserInputError


def human_summary(entry: dict[str, Any]) -> str:
    """Render one registered code for a terminal.

    A registered code carries many distinct messages (``ACPY-TYPE-006`` reports
    more than sixty), so a single hand-written paragraph cannot describe it and
    any attempt reads as a concrete-but-wrong statement. The exact message
    templates are listed instead, and a hand-written title is kept as the heading
    when the code has one.
    """

    code = str(entry.get("code", ""))
    title = entry.get("title")
    heading = (
        title
        if isinstance(title, str) and title
        else "one code, several reported conditions"
    )
    lines = [f"{code}: {heading}"]
    stage = entry.get("stage")
    if isinstance(stage, str) and stage:
        lines.append(f"  stage: {stage}")
    messages = entry.get("messages")
    if isinstance(messages, list) and messages:
        lines.append(f"  reported conditions ({len(messages)}):")
        lines.extend(f"    - {message}" for message in messages)
    return "\n".join(lines)


def run(arguments: object, sink: OutputSink) -> int:
    code = getattr(arguments, "code")
    entries = diagnostic_catalog().get("entries")
    if type(entries) is not list:
        raise ValueError("packaged diagnostic catalog is invalid")
    matches = [
        item for item in entries if type(item) is dict and item.get("code") == code
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
    entry = matches[0]
    document = {
        "schema": "agentic-circuit-diagnostic-explanation",
        "version": "0.1",
        **entry,
    }
    sink.result(document, human=human_summary(entry))
    return ExitCode.SUCCESS
