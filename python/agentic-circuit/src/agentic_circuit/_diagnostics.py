"""Immutable frontend diagnostics matching diagnostic.schema.json."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Literal

from ._canonical_json import JsonValue


Severity = Literal["error", "warning", "note"]
_CODE = re.compile(r"^AC(PY|ELAB|IR-[A-Z]+|LOWER|BUILD|TRACE|RUN)-[A-Z0-9-]+$")


@dataclass(frozen=True, order=True, slots=True)
class SourceSpan:
    file: str
    start_line: int
    start_column: int
    end_line: int
    end_column: int

    def __post_init__(self) -> None:
        if not self.file:
            raise ValueError("source file must not be empty")
        if (
            min(
                self.start_line,
                self.start_column,
                self.end_line,
                self.end_column,
            )
            < 1
        ):
            raise ValueError("source positions are one-based")
        if (self.end_line, self.end_column) < (
            self.start_line,
            self.start_column,
        ):
            raise ValueError("source span end precedes its start")

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "file": self.file,
            "line": self.start_line,
            "column": self.start_column,
        }


@dataclass(frozen=True, slots=True)
class RelatedLocation:
    message: str
    source: SourceSpan | None = None
    object_path: str | None = None

    def __post_init__(self) -> None:
        if not self.message:
            raise ValueError("related-location message must not be empty")

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "message": self.message,
            "source": self.source.to_json() if self.source is not None else None,
            "object_path": self.object_path,
        }


@dataclass(frozen=True, slots=True)
class FixIt:
    message: str

    def __post_init__(self) -> None:
        if not self.message:
            raise ValueError("fix-it message must not be empty")

    def to_json(self) -> dict[str, JsonValue]:
        return {"message": self.message}


@dataclass(frozen=True, slots=True)
class Diagnostic:
    stage: str
    code: str
    severity: Severity
    message: str
    source: SourceSpan | None = None
    object_path: str | None = None
    expected: JsonValue = None
    actual: JsonValue = None
    related: tuple[RelatedLocation, ...] = ()
    fixits: tuple[FixIt, ...] = ()
    schema: str = "agentic-circuit-diagnostic"
    version: str = "0.1"
    contract_epoch: str = "0.5"

    def __post_init__(self) -> None:
        if (
            self.schema,
            self.version,
            self.contract_epoch,
        ) != ("agentic-circuit-diagnostic", "0.1", "0.5"):
            raise ValueError("diagnostic schema identity is fixed at epoch 0.5")
        if not self.stage:
            raise ValueError("diagnostic stage must not be empty")
        if not _CODE.fullmatch(self.code):
            raise ValueError(f"invalid diagnostic code: {self.code!r}")
        if self.severity not in ("error", "warning", "note"):
            raise ValueError(f"invalid diagnostic severity: {self.severity!r}")
        if not self.message:
            raise ValueError("diagnostic message must not be empty")

    def sort_key(self) -> tuple[object, ...]:
        source = self.source
        return (
            source is None,
            source.file if source is not None else "",
            source.start_line if source is not None else 0,
            source.start_column if source is not None else 0,
            self.object_path or "",
            self.code,
            self.message,
        )

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "schema": self.schema,
            "version": self.version,
            "contract_epoch": self.contract_epoch,
            "code": self.code,
            "stage": self.stage,
            "severity": self.severity,
            "source": self.source.to_json() if self.source is not None else None,
            "object_path": self.object_path,
            "message": self.message,
            "expected": self.expected,
            "actual": self.actual,
            "related": [location.to_json() for location in self.related],
            "fixits": [fixit.to_json() for fixit in self.fixits],
        }


class DiagnosticBag:
    """Mutable collector with deterministic immutable snapshots."""

    __slots__ = ("_items",)

    def __init__(self) -> None:
        self._items: list[Diagnostic] = []

    def add(self, diagnostic: Diagnostic) -> None:
        self._items.append(diagnostic)

    def freeze(self) -> tuple[Diagnostic, ...]:
        return tuple(sorted(self._items, key=Diagnostic.sort_key))
