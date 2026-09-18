"""ACPy provenance construction for Queue/rule sources."""

from __future__ import annotations

import ast
import re
from dataclasses import dataclass
from typing import Mapping

from .._acpy import AcpyDocument, EntityAllocator, Property, SourceFile
from .._diagnostics import SourceSpan
from .errors import QueueFrontendError
from .syntax import _decorator_name


_NDF_DIRECT = re.compile(r"^#\s*ndf\s*:\s*(.*)$")
_NDF_REQUIRES = re.compile(r"^#\s*ndf\s*:\s*requires\s+(.*)$")
_NDF_ID_TEXT = r"[A-Z][A-Z0-9]*(?:-[A-Z0-9]+){2,}"
_NDF_ID = re.compile(_NDF_ID_TEXT)
_NDF_LIST = re.compile(rf"{_NDF_ID_TEXT}(?:\s*,\s*{_NDF_ID_TEXT})*")


@dataclass(frozen=True, slots=True)
class NdfMetadata:
    ids: tuple[str, ...] = ()
    requires: tuple[str, ...] = ()


DefinitionNdfMetadata = Mapping[str, NdfMetadata]


def extract_definition_ndf_metadata(text: str) -> dict[str, NdfMetadata]:
    """Extract adjacent machine-readable NDF comments from Python definitions."""

    tree = ast.parse(text, type_comments=True)
    lines = text.splitlines()
    result: dict[str, NdfMetadata] = {}
    for node in ast.walk(tree):
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
            continue
        first_line = min(
            (decorator.lineno for decorator in node.decorator_list),
            default=node.lineno,
        )
        cursor = first_line - 2
        comments: list[str] = []
        while cursor >= 0 and lines[cursor].lstrip().startswith("#"):
            comments.append(lines[cursor].strip())
            cursor -= 1
        direct: list[str] = []
        required: list[str] = []
        for comment in reversed(comments):
            requires = _NDF_REQUIRES.fullmatch(comment)
            direct_match = _NDF_DIRECT.fullmatch(comment)
            target = required if requires is not None else direct
            content = (
                requires.group(1)
                if requires is not None
                else direct_match.group(1)
                if direct_match is not None
                else ""
            )
            normalized = content.strip()
            identifiers = _NDF_ID.findall(normalized)
            if (requires is not None or direct_match is not None) and (
                not identifiers or _NDF_LIST.fullmatch(normalized) is None
            ):
                raise QueueFrontendError(
                    f"ACPY-NDF-001: definition {node.name!r} has an invalid NDF comment"
                )
            for identifier in identifiers:
                if identifier not in target:
                    target.append(identifier)
        if not direct and not required:
            continue
        metadata = NdfMetadata(tuple(direct), tuple(required))
        previous = result.get(node.name)
        if previous is not None and previous != metadata:
            raise QueueFrontendError(
                f"ACPY-NDF-001: definition {node.name!r} has ambiguous NDF metadata"
            )
        result[node.name] = metadata
    return result


def build_queue_acpy(text: str, system: str, source_path: str) -> AcpyDocument:
    """Build the minimal verified ACPy provenance for a Queue/rule source."""

    tree = ast.parse(text, filename=source_path, type_comments=True)
    systems = [
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and node.name == system
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "system"
            for decorator in node.decorator_list
        )
    ]
    if len(systems) != 1:
        raise QueueFrontendError(
            f"ACPY-QUEUE-001: system {system!r} is missing or ambiguous"
        )

    def span(node: ast.AST) -> SourceSpan:
        return SourceSpan(
            source_path,
            node.lineno,
            node.col_offset + 1,
            getattr(node, "end_lineno", node.lineno),
            getattr(node, "end_col_offset", node.col_offset) + 1,
        )

    allocator = EntityAllocator()
    system_entity = allocator.allocate(
        kind="system",
        scope=system,
        source=span(systems[0]),
        properties=(Property("frontend", "queue_rule"),),
    )
    for node in tree.body:
        if not isinstance(node, ast.FunctionDef) or not any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "rule"
            for decorator in node.decorator_list
        ):
            continue
        allocator.allocate(
            kind="rule",
            scope=f"{system}.{node.name}",
            source=span(node),
            parent=system_entity.id,
            properties=(Property("name", node.name),),
        )
    document = AcpyDocument(
        entry=system_entity.id,
        sources=(SourceFile(source_path),),
        entities=allocator.freeze(),
    )
    errors = document.verify()
    if errors:
        raise QueueFrontendError(
            "ACPY-VERIFY-001: " + "; ".join(error.message for error in errors)
        )
    return document
