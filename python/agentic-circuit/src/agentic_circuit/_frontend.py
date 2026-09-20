"""Composition boundary for deterministic definition capture."""

from __future__ import annotations

import ast
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

from ._definitions import Definition
from ._diagnostics import (
    Diagnostic,
    DiagnosticBag,
    SourceSpan,
)
from ._schemas import SchemaRegistry
from ._source import DefinitionSite, SourceUnit, load_source_unit
from ._static_eval import StaticEnvironment, StaticValue, evaluate_static


@dataclass(frozen=True, slots=True)
class CaptureRequest:
    entry: Path
    workspace: Path
    system: str
    static_arguments: tuple[tuple[str, StaticValue], ...] = ()


@dataclass(frozen=True, slots=True)
class CapturedProgram:
    source: SourceUnit
    definitions: tuple[Definition, ...]
    selected_system: Definition | None
    registry: SchemaRegistry
    static_arguments: tuple[tuple[str, StaticValue], ...] = ()
    diagnostics: tuple[Diagnostic, ...] = ()


def _diagnostic(
    bag: DiagnosticBag, code: str, message: str, site: DefinitionSite | None = None
) -> None:
    bag.add(
        Diagnostic(
            stage="definition-capture",
            code=code,
            severity="error",
            message=message,
            source=site.span if site is not None else None,
        )
    )


def _decorator_options(
    site: DefinitionSite, kind: str
) -> tuple[tuple[str, object], ...] | None:
    for decorator in site.node.decorator_list:
        candidate = decorator.func if isinstance(decorator, ast.Call) else decorator
        if not isinstance(candidate, ast.Name) or candidate.id != kind:
            continue
        if not isinstance(decorator, ast.Call):
            return ()
        if decorator.args or any(keyword.arg is None for keyword in decorator.keywords):
            return None
        options: list[tuple[str, object]] = []
        try:
            for keyword in decorator.keywords:
                assert keyword.arg is not None
                options.append(
                    (
                        keyword.arg,
                        evaluate_static(keyword.value, StaticEnvironment({})),
                    )
                )
        except ValueError:
            return None
        return tuple(sorted(options))
    return None


def _all_arguments(node: ast.FunctionDef | ast.AsyncFunctionDef) -> list[ast.arg]:
    return [
        *node.args.posonlyargs,
        *node.args.args,
        *node.args.kwonlyargs,
        *([node.args.vararg] if node.args.vararg is not None else []),
        *([node.args.kwarg] if node.args.kwarg is not None else []),
    ]


def _symbolic_annotation(annotation: ast.expr) -> bool:
    value = annotation.value if isinstance(annotation, ast.Subscript) else annotation
    return isinstance(value, ast.Name) and value.id in {
        "Flow",
        "Endpoint",
        "ResourceRef",
    }


def _validate_signature(
    definition: Definition, site: DefinitionSite, diagnostics: DiagnosticBag
) -> None:
    if not isinstance(site.node, (ast.FunctionDef, ast.AsyncFunctionDef)):
        _diagnostic(
            diagnostics,
            "ACPY-TYPE-DEFINITION",
            f"{definition.qualified_name!r} must decorate a function",
            site,
        )
        return
    if site.node.args.vararg is not None or site.node.args.kwarg is not None:
        _diagnostic(
            diagnostics,
            "ACPY-TYPE-SIGNATURE",
            f"{definition.qualified_name!r} cannot use variadic public parameters",
            site,
        )
    for argument in _all_arguments(site.node):
        if argument.annotation is None:
            _diagnostic(
                diagnostics,
                "ACPY-TYPE-ANNOTATION",
                f"public parameter {argument.arg!r} requires an annotation",
                site,
            )
        elif definition.kind == "system" and _symbolic_annotation(argument.annotation):
            _diagnostic(
                diagnostics,
                "ACPY-TYPE-SYSTEM",
                f"system parameter {argument.arg!r} must be static",
                site,
            )
    defaults = [
        *site.node.args.defaults,
        *(default for default in site.node.args.kw_defaults if default is not None),
    ]
    for default in defaults:
        try:
            evaluate_static(default, StaticEnvironment({}))
        except ValueError:
            _diagnostic(
                diagnostics,
                "ACPY-STATIC-DEFAULT",
                f"{definition.qualified_name!r} has a non-static default",
                site,
            )
    if site.node.returns is None:
        _diagnostic(
            diagnostics,
            "ACPY-TYPE-RETURN",
            f"{definition.qualified_name!r} requires a return annotation",
            site,
        )


def _match_definitions(
    namespace: Mapping[str, object],
    unit: SourceUnit,
    entry: Path,
    diagnostics: DiagnosticBag,
) -> tuple[Definition, ...]:
    sites = {
        (site.qualified_name, site.span.start_line): site for site in unit.definitions
    }
    module_name = namespace.get("__name__")
    found: list[Definition] = []
    seen: set[int] = set()
    for value in namespace.values():
        if not isinstance(value, Definition) or id(value) in seen:
            continue
        seen.add(id(value))
        key = (value.qualified_name, value.source_line)
        site = sites.get(key)
        source_matches = (
            value.source_file is not None
            and Path(value.source_file).resolve() == entry.resolve()
        )
        options = _decorator_options(site, value.kind) if site is not None else None
        if (
            site is None
            or not source_matches
            or value.module_name != module_name
            or options != value.explicit_options
        ):
            _diagnostic(
                diagnostics,
                "ACPY-SYMBOL-DEFINITION",
                f"registered definition {value.qualified_name!r} does not match source",
                site,
            )
            continue
        _validate_signature(value, site, diagnostics)
        found.append(value)
    return tuple(
        sorted(found, key=lambda item: (item.source_line or 0, item.qualified_name))
    )


def capture_definitions(
    request: CaptureRequest,
    namespace: Mapping[str, object],
    registry: SchemaRegistry,
) -> CapturedProgram:
    unit = load_source_unit(request.entry, request.workspace)
    diagnostics = DiagnosticBag()
    definitions = _match_definitions(namespace, unit, request.entry, diagnostics)
    candidates = [
        definition
        for definition in definitions
        if definition.kind == "system" and definition.qualified_name == request.system
    ]
    selected: Definition | None = None
    if len(candidates) == 1:
        selected = candidates[0]
    elif not candidates:
        _diagnostic(
            diagnostics,
            "ACPY-SYMBOL-SYSTEM",
            f"system {request.system!r} was not found",
        )
    else:
        _diagnostic(
            diagnostics,
            "ACPY-SYMBOL-SYSTEM",
            f"system {request.system!r} is ambiguous",
        )
    return CapturedProgram(
        source=unit,
        definitions=definitions,
        selected_system=selected,
        registry=registry,
        static_arguments=request.static_arguments,
        diagnostics=diagnostics.freeze(),
    )


def construct_captured_process(
    captured: CapturedProgram, definition: str, effects: object
):
    """Construct one captured ``@process`` without executing its Python body."""

    from ._process import EffectRegistry, ProcessConstructionError, construct_process

    if not isinstance(effects, EffectRegistry):
        raise TypeError("effects must be an EffectRegistry")
    matches = [
        site
        for site in captured.source.definitions
        if site.qualified_name == definition or site.name == definition
    ]
    if len(matches) != 1:
        raise ProcessConstructionError(
            "ACPY-PROCESS-001",
            f"captured process {definition!r} is missing or ambiguous",
            (
                captured.source.definitions[0].span
                if captured.source.definitions
                else SourceSpan(captured.source.path, 1, 1, 1, 1)
            ),
        )
    return construct_process(matches[0], effects)
