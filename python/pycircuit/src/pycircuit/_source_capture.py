"""Private Python source capture without model execution."""

from __future__ import annotations

import ast
import tokenize
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True, slots=True)
class _CapturedSourceSpan:
    """A source range with one-based UTF-8 byte and code point columns."""

    path: Path
    line: int
    byte_column: int
    codepoint_column: int
    end_line: int
    end_byte_column: int
    end_codepoint_column: int


@dataclass(frozen=True, slots=True)
class _CapturedSourceFile:
    """Decoded source text and its unexecuted Python syntax tree."""

    source_root: Path
    path: Path
    source: str
    encoding: str
    syntax: ast.Module

    def span_for(self, node: ast.AST) -> _CapturedSourceSpan:
        return _source_span_for_node(self, node)


def _position(node: ast.AST, attribute: str) -> int:
    value = getattr(node, attribute, None)
    if type(value) is not int:
        raise ValueError(f"AST node has invalid {attribute}: {value!r}")
    return value


def _codepoint_column(
    lines: list[str], line: int, byte_column: int, attribute: str
) -> int:
    if line < 1 or line > len(lines):
        raise ValueError(f"AST node has invalid {attribute} line: {line!r}")
    if byte_column < 0:
        raise ValueError(f"AST node has invalid {attribute}: {byte_column!r}")

    encoded_line = lines[line - 1].encode("utf-8")
    if byte_column > len(encoded_line):
        raise ValueError(f"AST node has invalid {attribute}: {byte_column!r}")
    try:
        prefix = encoded_line[:byte_column].decode("utf-8")
    except UnicodeDecodeError as error:
        raise ValueError(
            f"AST node {attribute} is not on a Unicode code point boundary: "
            f"{byte_column!r}"
        ) from error
    return len(prefix) + 1


def _source_span_for_node(
    captured: _CapturedSourceFile, node: ast.AST
) -> _CapturedSourceSpan:
    """Convert CPython AST positions into validated one-based columns."""

    line = _position(node, "lineno")
    byte_column = _position(node, "col_offset")
    end_line = _position(node, "end_lineno")
    end_byte_column = _position(node, "end_col_offset")

    lines = captured.source.split("\n")
    codepoint_column = _codepoint_column(lines, line, byte_column, "col_offset")
    end_codepoint_column = _codepoint_column(
        lines, end_line, end_byte_column, "end_col_offset"
    )

    if end_line < line or (end_line == line and end_byte_column < byte_column):
        raise ValueError("AST node source span ends before it starts")

    return _CapturedSourceSpan(
        path=captured.path,
        line=line,
        byte_column=byte_column + 1,
        codepoint_column=codepoint_column,
        end_line=end_line,
        end_byte_column=end_byte_column + 1,
        end_codepoint_column=end_codepoint_column,
    )


def _capture_source_file(
    path: str | Path, *, source_root: str | Path
) -> _CapturedSourceFile:
    """Capture one Python file confined to ``source_root`` without executing it."""

    resolved_root = Path(source_root).resolve(strict=True)
    if not resolved_root.is_dir():
        raise ValueError(f"source root is not a directory: {resolved_root}")

    requested_path = Path(path)
    if not requested_path.is_absolute():
        requested_path = resolved_root / requested_path
    resolved_path = requested_path.resolve(strict=True)
    try:
        resolved_path.relative_to(resolved_root)
    except ValueError as error:
        raise ValueError(f"source file escapes source root: {resolved_path}") from error
    if not resolved_path.is_file():
        raise ValueError(f"source path is not a file: {resolved_path}")

    with tokenize.open(resolved_path) as source_file:
        source = source_file.read()
        encoding = source_file.encoding
    syntax = ast.parse(source, filename=str(resolved_path), type_comments=True)

    return _CapturedSourceFile(
        source_root=resolved_root,
        path=resolved_path,
        source=source,
        encoding=encoding,
        syntax=syntax,
    )
