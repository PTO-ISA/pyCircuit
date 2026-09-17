"""ACPy provenance construction for Queue/rule sources."""

from __future__ import annotations

import ast

from .._acpy import AcpyDocument, EntityAllocator, Property, SourceFile
from .._canonical_json import sha256_bytes
from .._diagnostics import SourceSpan
from .errors import QueueFrontendError
from .syntax import _decorator_name


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
        sources=(SourceFile(source_path, sha256_bytes(text.encode("utf-8"))),),
        entities=allocator.freeze(),
    )
    errors = document.verify()
    if errors:
        raise QueueFrontendError(
            "ACPY-VERIFY-001: " + "; ".join(error.message for error in errors)
        )
    return document
