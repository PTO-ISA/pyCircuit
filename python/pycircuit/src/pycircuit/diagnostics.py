from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

PYTHON_DIAGNOSTIC_CODES = frozenset(
    {
        "PYC-PY-CONNECTOR",
        "PYC-PY-DESIGN",
        "PYC-PY-DIAGNOSTIC",
        "PYC-PY-JIT",
        "PYC-PY-KEY",
        "PYC-PY-PROBE",
        "PYC-PY-RUNTIME",
        "PYC-PY-TB",
        "PYC-PY-TRACE",
        "PYC-PY-TYPE",
        "PYC-PY-VALUE",
    }
)


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

    def __post_init__(self) -> None:
        if self.code.startswith("PYC-PY-") and self.code not in PYTHON_DIAGNOSTIC_CODES:
            raise ValueError(
                f"unregistered pyCircuit Python diagnostic code: {self.code}"
            )

    @property
    def location(self) -> str:
        """Render the diagnostic source location in the public CLI format."""
        return location_string(self.path, self.line, self.col)


class PyCircuitError(RuntimeError):
    """Base class for public pyCircuit runtime and authoring failures."""

    default_code = "PYC-PY-RUNTIME"
    default_stage = "python"

    def __init__(
        self,
        message: str | Diagnostic,
        *,
        code: str | None = None,
        stage: str | None = None,
        path: str | None = None,
        line: int | None = None,
        col: int | None = None,
        hint: str | None = None,
        snippet: str | None = None,
    ) -> None:
        if isinstance(message, Diagnostic):
            diagnostic = message
        else:
            diagnostic = make_diagnostic(
                code=code or self.default_code,
                stage=stage or self.default_stage,
                message=str(message),
                path=path,
                line=line,
                col=col,
                hint=hint,
                snippet=snippet,
            )
        self.diagnostic = diagnostic
        super().__init__(render_diagnostic(diagnostic))

    def __str__(self) -> str:
        # KeyError normally quotes its sole argument; all pyCircuit errors use
        # one stable diagnostic rendering regardless of built-in compatibility.
        return render_diagnostic(self.diagnostic)

    @property
    def code(self) -> str:
        return self.diagnostic.code

    @property
    def message(self) -> str:
        return self.diagnostic.message

    @property
    def location(self) -> str:
        return self.diagnostic.location


class PyCircuitTypeError(PyCircuitError, TypeError):
    """A public pyCircuit argument/type error with ``TypeError`` compatibility."""

    default_code = "PYC-PY-TYPE"


class PyCircuitValueError(PyCircuitError, ValueError):
    """A public pyCircuit value error with ``ValueError`` compatibility."""

    default_code = "PYC-PY-VALUE"


class PyCircuitKeyError(PyCircuitError, KeyError):
    """A public pyCircuit lookup error with ``KeyError`` compatibility."""

    default_code = "PYC-PY-KEY"


class DiagnosticError(PyCircuitError):
    default_code = "PYC-PY-DIAGNOSTIC"

    def __init__(self, diagnostic: Diagnostic) -> None:
        super().__init__(diagnostic)


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
