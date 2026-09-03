from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Diagnostic:
    code: str
    stage: str
    message: str
    path: str | None = None
    line: int | None = None
    col: int | None = None
    hint: str | None = None
    snippet: str | None = None


class DiagnosticError(RuntimeError):
    def __init__(self, diagnostic: Diagnostic) -> None:
        self.diagnostic = diagnostic
        super().__init__(render_diagnostic(diagnostic))


def location_string(path: str | None, line: int | None, col: int | None) -> str:
    p = path or "<unknown>"
    if line is None:
        return p
    if col is None:
        return f"{p}:{int(line)}"
    return f"{p}:{int(line)}:{int(col)}"


def _line_from_text(text: str, line: int) -> str | None:
    if line <= 0:
        return None
    lines = text.splitlines()
    idx = int(line) - 1
    if idx < 0 or idx >= len(lines):
        return None
    return lines[idx]


def snippet_from_text(text: str, line: int | None) -> str | None:
    if line is None:
        return None
    s = _line_from_text(text, int(line))
    if s is None:
        return None
    return s.rstrip("\n")


def snippet_from_file(path: Path, line: int | None) -> str | None:
    if line is None:
        return None
    try:
        text = path.read_text(encoding="utf-8")
    except Exception:
        return None
    return snippet_from_text(text, line)


def render_diagnostic(d: Diagnostic) -> str:
    loc = location_string(d.path, d.line, d.col)
    header = f"{loc}: [{d.code}] {d.message}"
    stage = f"stage={d.stage}"
    parts = [header, stage]
    if d.snippet:
        parts.append(f"  | {d.snippet}")
    if d.hint:
        parts.append(f"hint: {d.hint}")
    return "\n".join(parts)


def make_diagnostic(
    *,
    code: str,
    stage: str,
    message: str,
    path: str | None = None,
    line: int | None = None,
    col: int | None = None,
    hint: str | None = None,
    snippet: str | None = None,
) -> Diagnostic:
    return Diagnostic(
        code=str(code),
        stage=str(stage),
        message=str(message),
        path=(None if path is None else str(path)),
        line=(None if line is None else int(line)),
        col=(None if col is None else int(col)),
        hint=(None if hint is None else str(hint)),
        snippet=(None if snippet is None else str(snippet)),
    )
