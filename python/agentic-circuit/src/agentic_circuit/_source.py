"""Normalized Python source loading and lexical definition indexing."""

from __future__ import annotations

import ast
from dataclasses import dataclass
from pathlib import Path
from typing import TypeAlias

from ._canonical_json import sha256_bytes
from ._diagnostics import SourceSpan


DefinitionNode: TypeAlias = ast.FunctionDef | ast.AsyncFunctionDef | ast.ClassDef


class SourceCaptureError(ValueError):
    """Raised when a source file cannot enter the portable frontend."""


@dataclass(frozen=True, slots=True)
class DefinitionSite:
    qualified_name: str
    name: str
    lexical_index: int
    span: SourceSpan
    decorator_names: tuple[str, ...]
    node: DefinitionNode


@dataclass(frozen=True, slots=True)
class SourceUnit:
    path: str
    sha256: str
    text: str
    tree: ast.Module
    definitions: tuple[DefinitionSite, ...]


def _decorator_name(node: ast.expr) -> str:
    if isinstance(node, ast.Call):
        return _decorator_name(node.func)
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        prefix = _decorator_name(node.value)
        return f"{prefix}.{node.attr}" if prefix else node.attr
    return ""


def _source_span(node: DefinitionNode, path: str) -> SourceSpan:
    end_line = getattr(node, "end_lineno", None)
    end_column = getattr(node, "end_col_offset", None)
    if end_line is None or end_column is None:
        raise SourceCaptureError(f"unmatchable definition span for {node.name!r}")
    starts = [(node.lineno, node.col_offset)]
    starts.extend(
        (decorator.lineno, max(0, decorator.col_offset - 1))
        for decorator in node.decorator_list
    )
    start_line, start_column = min(starts)
    return SourceSpan(
        file=path,
        start_line=start_line,
        start_column=start_column + 1,
        end_line=end_line,
        end_column=end_column + 1,
    )


class _DefinitionIndexer(ast.NodeVisitor):
    def __init__(self, path: str) -> None:
        self._path = path
        self._scopes: list[tuple[str, bool]] = []
        self._seen: set[str] = set()
        self.sites: list[DefinitionSite] = []

    def _qualified_name(self, name: str) -> str:
        parts: list[str] = []
        for scope_name, is_function in self._scopes:
            parts.append(scope_name)
            if is_function:
                parts.append("<locals>")
        parts.append(name)
        return ".".join(parts)

    def _visit_definition(self, node: DefinitionNode, *, is_function: bool) -> None:
        qualified_name = self._qualified_name(node.name)
        if qualified_name in self._seen:
            raise SourceCaptureError(
                f"duplicate definition {qualified_name!r} in {self._path}"
            )
        self._seen.add(qualified_name)
        self.sites.append(
            DefinitionSite(
                qualified_name=qualified_name,
                name=node.name,
                lexical_index=len(self.sites),
                span=_source_span(node, self._path),
                decorator_names=tuple(
                    _decorator_name(decorator) for decorator in node.decorator_list
                ),
                node=node,
            )
        )
        self._scopes.append((node.name, is_function))
        self.generic_visit(node)
        self._scopes.pop()

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        self._visit_definition(node, is_function=True)

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        self._visit_definition(node, is_function=True)

    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        self._visit_definition(node, is_function=False)


def index_definitions(tree: ast.Module, path: str) -> tuple[DefinitionSite, ...]:
    indexer = _DefinitionIndexer(path)
    indexer.visit(tree)
    return tuple(indexer.sites)


def load_source_unit(entry: Path, workspace: Path) -> SourceUnit:
    root = workspace.resolve(strict=True)
    source = entry.resolve(strict=True)
    try:
        relative = source.relative_to(root).as_posix()
    except ValueError as error:
        raise SourceCaptureError(
            f"source {source} is outside workspace {root}"
        ) from error

    raw = source.read_bytes()
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as error:
        raise SourceCaptureError(f"source {relative!r} is not UTF-8") from error
    tree = ast.parse(text, filename=relative, type_comments=True)
    return SourceUnit(
        path=relative,
        sha256=sha256_bytes(raw),
        text=text,
        tree=tree,
        definitions=index_definitions(tree, relative),
    )
