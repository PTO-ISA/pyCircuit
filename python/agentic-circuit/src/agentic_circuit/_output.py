"""Deterministic human and structured CLI output routing."""

from __future__ import annotations

import sys
from dataclasses import dataclass, field
from typing import Iterable, Literal, TextIO

from ._canonical_json import JsonValue, canonical_json_bytes
from ._diagnostics import Diagnostic


OutputFormat = Literal["text", "json", "jsonl"]


@dataclass(slots=True)
class OutputSink:
    format: OutputFormat = "text"
    quiet: bool = False
    stdout: TextIO = field(default_factory=lambda: sys.stdout)
    stderr: TextIO = field(default_factory=lambda: sys.stderr)
    _single_json_written: bool = False

    @classmethod
    def from_arguments(
        cls, arguments: object, *, workspace_format: OutputFormat | None = None
    ) -> "OutputSink":
        explicit = getattr(arguments, "diagnostic_format", None)
        selected: OutputFormat = explicit or workspace_format or "text"
        if getattr(arguments, "json", False):
            selected = "json"
        return cls(format=selected, quiet=bool(getattr(arguments, "quiet", False)))

    def _json_line(self, value: JsonValue) -> str:
        return canonical_json_bytes(value).decode("utf-8") + "\n"

    def result(self, value: JsonValue, *, human: str = "") -> None:
        if self.format == "json":
            if self._single_json_written:
                raise RuntimeError("single-JSON stdout already contains a value")
            self.stdout.write(self._json_line(value))
            self._single_json_written = True
        elif self.format == "jsonl":
            self.stdout.write(self._json_line(value))
        elif human and not self.quiet:
            self.stdout.write(human if human.endswith("\n") else human + "\n")

    def diagnostics(self, diagnostics: Iterable[Diagnostic]) -> None:
        ordered = tuple(sorted(diagnostics, key=Diagnostic.sort_key))
        if not ordered:
            return
        if self.format == "json":
            if len(ordered) == 1:
                self.result(ordered[0].to_json())
            else:
                self.result([item.to_json() for item in ordered])
            return
        if self.format == "jsonl":
            for diagnostic in ordered:
                self.stdout.write(self._json_line(diagnostic.to_json()))
            return
        if self.quiet:
            return
        for diagnostic in ordered:
            location = ""
            if diagnostic.source is not None:
                location = (
                    f"{diagnostic.source.file}:{diagnostic.source.start_line}:"
                    f"{diagnostic.source.start_column}: "
                )
            self.stderr.write(
                f"{location}{diagnostic.severity}: {diagnostic.code}: "
                f"{diagnostic.message}\n"
            )

    @staticmethod
    def bounded_capture(text: str, *, limit: int = 1 << 20) -> tuple[str, bool]:
        if limit < 0:
            raise ValueError("capture limit must be nonnegative")
        encoded = text.encode("utf-8")
        if len(encoded) <= limit:
            return text, False
        clipped = encoded[:limit]
        while True:
            try:
                return clipped.decode("utf-8"), True
            except UnicodeDecodeError as error:
                clipped = clipped[: error.start]
