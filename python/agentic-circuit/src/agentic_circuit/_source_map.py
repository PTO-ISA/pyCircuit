"""Stable AST-node source origins across deterministic closure flattening."""

from __future__ import annotations

import ast
from collections.abc import Iterator, Mapping
from dataclasses import dataclass
from typing import TypeAlias


@dataclass(frozen=True, slots=True)
class SourceFrame:
    file: str
    line: int
    column: int
    end_line: int
    end_column: int


SourceNodeRecord: TypeAlias = tuple[str, SourceFrame]
SourceNodeLocations: TypeAlias = Mapping[str, tuple[SourceNodeRecord, ...]]


def walk_ast_paths(node: ast.AST) -> Iterator[tuple[str, ast.AST]]:
    """Yield deterministic field/index paths rooted at one definition."""

    def visit(current: ast.AST, path: str) -> Iterator[tuple[str, ast.AST]]:
        yield path, current
        for field_name, value in ast.iter_fields(current):
            field_path = f"{path}.{field_name}" if path else field_name
            if isinstance(value, ast.AST):
                yield from visit(value, field_path)
                continue
            if isinstance(value, list):
                for index, item in enumerate(value):
                    if isinstance(item, ast.AST):
                        yield from visit(item, f"{field_path}[{index}]")

    yield from visit(node, "")


def capture_source_node_locations(
    tree: ast.Module,
    source_path: str,
) -> dict[str, tuple[SourceNodeRecord, ...]]:
    """Capture original locations without using object identity or display names."""

    result: dict[str, tuple[SourceNodeRecord, ...]] = {}
    for definition in tree.body:
        if not isinstance(
            definition,
            (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef),
        ):
            continue
        records: list[SourceNodeRecord] = []
        for path, node in walk_ast_paths(definition):
            line = getattr(node, "lineno", None)
            column = getattr(node, "col_offset", None)
            if line is None or column is None:
                continue
            end_line = getattr(node, "end_lineno", line)
            end_column = getattr(node, "end_col_offset", column + 1)
            records.append(
                (
                    path,
                    SourceFrame(
                        source_path,
                        line,
                        column + 1,
                        end_line,
                        max(1, end_column + 1),
                    ),
                )
            )
        result[definition.name] = tuple(records)
    return result


def apply_source_node_locations(
    tree: ast.Module,
    locations: SourceNodeLocations | None,
    fallback_path: str,
) -> None:
    """Attach immutable source frames to the reparsed flattened AST."""

    captured = locations or {}
    for definition in tree.body:
        if not isinstance(
            definition,
            (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef),
        ):
            continue
        by_path = dict(captured.get(definition.name, ()))
        for path, node in walk_ast_paths(definition):
            frame = by_path.get(path)
            if frame is None:
                if not fallback_path.endswith(".py"):
                    continue
                line = getattr(node, "lineno", None)
                column = getattr(node, "col_offset", None)
                if line is None or column is None:
                    continue
                end_line = getattr(node, "end_lineno", line)
                end_column = getattr(node, "end_col_offset", column + 1)
                frame = SourceFrame(
                    fallback_path,
                    line,
                    column + 1,
                    end_line,
                    max(1, end_column + 1),
                )
            setattr(node, "_ac_source_frame", frame)


def source_frame(node: ast.AST) -> SourceFrame | None:
    frame = getattr(node, "_ac_source_frame", None)
    return frame if isinstance(frame, SourceFrame) else None
