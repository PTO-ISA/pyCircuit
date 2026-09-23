"""Fresh-process trusted project capture and verified result transport."""

from __future__ import annotations

import argparse
import ast
import contextlib
import importlib.util
import io
import json
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from types import ModuleType

try:
    import _pycircuit_semantics
except ModuleNotFoundError:
    _pycircuit_semantics = None

from ._canonical_json import JsonValue, canonical_json_bytes
from ._definitions import Definition
from ._diagnostics import (
    Diagnostic,
    FixIt,
    RelatedLocation,
    SourceSpan,
    diagnostic_from_exception,
)
from ._output import OutputSink
from ._queue_compiler.provenance import DefinitionNdfMetadata, NdfMetadata
from ._source_closure import SourceClosure
from ._source_map import SourceNodeRecord, capture_source_node_locations
from ._staging import ArtifactStage
from ._static_eval import FrozenMap, StaticValue


@dataclass(frozen=True, slots=True)
class CaptureWorkerRequest:
    python: str
    workspace: Path
    entry: Path
    system: str
    static_arguments: tuple[tuple[str, JsonValue], ...]
    component_roots: tuple[Path, ...]
    private_output: Path
    timeout: float = 30.0
    source_closure: bool = False
    module_name: str | None = None
    entry_kind: str = "system"
    source_modules: tuple[str, ...] = ()


@dataclass(frozen=True, slots=True)
class CaptureWorkerResult:
    acpy: bytes | None
    acir: bytes | None
    diagnostics: tuple[Diagnostic, ...]
    project_report: bytes | None
    frontend_kind: str | None


_STAGE_FILES = (
    "model.ac.mlir",
    "model.acpy.json",
    "project-report.txt",
    "request.json",
    "result.json",
)


def _source(value: object) -> SourceSpan | None:
    if value is None:
        return None
    if type(value) is not dict or set(value) != {"file", "line", "column"}:
        raise ValueError("worker diagnostic source is invalid")
    return SourceSpan(
        str(value["file"]),
        int(value["line"]),
        int(value["column"]),
        int(value["line"]),
        int(value["column"]),
    )


def _diagnostic(value: object) -> Diagnostic:
    if type(value) is not dict:
        raise ValueError("worker diagnostic is invalid")
    related = tuple(
        RelatedLocation(
            message=item["message"],
            source=_source(item["source"]),
            object_path=item["object_path"],
        )
        for item in value["related"]
    )
    fixits = tuple(FixIt(item["message"]) for item in value["fixits"])
    return Diagnostic(
        stage=value["stage"],
        code=value["code"],
        severity=value["severity"],
        message=value["message"],
        source=_source(value["source"]),
        object_path=value["object_path"],
        expected=value["expected"],
        actual=value["actual"],
        related=related,
        fixits=fixits,
    )


def _failure(code: str, message: str) -> CaptureWorkerResult:
    return CaptureWorkerResult(
        acpy=None,
        acir=None,
        diagnostics=(
            Diagnostic(
                stage="frontend-capture",
                code=code,
                severity="error",
                message=message,
            ),
        ),
        project_report=None,
        frontend_kind=None,
    )


def _request_json(request: CaptureWorkerRequest, output: Path) -> dict[str, JsonValue]:
    return {
        "schema": "agentic-circuit-capture-request",
        "version": "0.1",
        "workspace": request.workspace.resolve().as_posix(),
        "entry": request.entry.resolve().as_posix(),
        "system": request.system,
        "static_arguments": dict(request.static_arguments),
        "component_roots": [
            path.resolve().relative_to(request.workspace.resolve()).as_posix()
            for path in request.component_roots
        ],
        "source_closure": request.source_closure,
        "module_name": request.module_name,
        "entry_kind": request.entry_kind,
        "source_modules": list(request.source_modules),
        "output": output.resolve().as_posix(),
    }


def run_capture_worker(request: CaptureWorkerRequest) -> CaptureWorkerResult:
    # Namespace-package path order is the import authority chosen by the
    # parent process. Preserve it so the isolated worker cannot prefer a stale
    # build/install copy merely because its path sorts before the source tree.
    agentic_roots = [
        str(Path(location).resolve().parent)
        for location in sys.modules["agentic_circuit"].__path__
    ]
    import_roots = list(agentic_roots[:1])
    if _pycircuit_semantics is not None:
        import_roots.append(
            str(Path(_pycircuit_semantics.__file__).resolve().parent.parent)
        )
    import_roots.extend(agentic_roots[1:])
    package_parents = os.pathsep.join(dict.fromkeys(import_roots))
    bootstrap = (
        "import os,runpy,sys;"
        "sys.path[:0]=sys.argv.pop(1).split(os.pathsep);"
        "runpy.run_module('agentic_circuit._capture_worker',run_name='__main__')"
    )
    with ArtifactStage(request.private_output, expected=_STAGE_FILES) as stage:
        assert stage.path is not None
        request_path = stage.path / "request.json"
        request_path.write_bytes(
            canonical_json_bytes(_request_json(request, stage.path))
        )
        environment = {
            "PATH": os.environ.get("PATH", ""),
            "PYTHONHASHSEED": "0",
        }
        try:
            completed = subprocess.run(
                (
                    request.python,
                    "-I",
                    "-c",
                    bootstrap,
                    package_parents,
                    "--request",
                    os.fspath(request_path),
                ),
                stdin=subprocess.DEVNULL,
                capture_output=True,
                timeout=request.timeout,
                check=False,
                env=environment,
            )
        except subprocess.TimeoutExpired:
            return _failure("ACPY-CAPTURE-002", "frontend capture timed out")
        if completed.returncode != 0:
            detail, _ = OutputSink.bounded_capture(
                (completed.stderr or completed.stdout).decode("utf-8", errors="replace")
            )
            return _failure(
                "ACPY-CAPTURE-001",
                "frontend capture worker failed" + (f": {detail}" if detail else ""),
            )
        try:
            stage.verify()
            response = json.loads((stage.path / "result.json").read_text())
            if set(response) != {
                "schema",
                "version",
                "has_acpy",
                "has_acir",
                "diagnostics",
                "frontend_kind",
            }:
                raise ValueError("worker result fields are invalid")
            diagnostics = tuple(_diagnostic(item) for item in response["diagnostics"])
            acpy = (
                (stage.path / "model.acpy.json").read_bytes()
                if response["has_acpy"]
                else None
            )
            acir = (
                (stage.path / "model.ac.mlir").read_bytes()
                if response["has_acir"]
                else None
            )
            report = (stage.path / "project-report.txt").read_bytes() or None
        except (OSError, UnicodeError, ValueError, KeyError, TypeError) as error:
            return _failure("ACPY-CAPTURE-001", f"capture result is invalid: {error}")
        frontend_kind = response["frontend_kind"]
        if frontend_kind not in {"structural", "queue_rule"}:
            raise ValueError("worker frontend kind is invalid")
        return CaptureWorkerResult(acpy, acir, diagnostics, report, frontend_kind)


def _load_project(
    entry: Path, workspace: Path, module_name: str | None = None
) -> dict[str, object]:
    sys.path.insert(1, os.fspath(workspace))
    selected_name = module_name or "_agentic_architecture"
    search_locations = (
        [os.fspath(entry.parent)] if entry.name == "__init__.py" else None
    )
    spec = importlib.util.spec_from_file_location(
        selected_name, entry, submodule_search_locations=search_locations
    )
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load architecture entry {entry}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return vars(module)


def _worker_diagnostic(error: BaseException, entry: Path) -> Diagnostic:
    if isinstance(error, SyntaxError):
        line = error.lineno or 1
        column = error.offset or 1
        source = SourceSpan(entry.name, line, column, line, column)
        default_code = "ACPY-SYNTAX-SOURCE"
    else:
        source = None
        default_code = "ACPY-CAPTURE-001"
    return diagnostic_from_exception(
        error,
        stage="frontend-capture",
        default_code=default_code,
        source=source,
        message_prefix="trusted project execution failed: ",
    )


def _static_value(value: JsonValue) -> StaticValue:
    if value is None or type(value) in (bool, int, float, str):
        return value
    if type(value) is list:
        return tuple(_static_value(item) for item in value)
    if type(value) is dict:
        return FrozenMap(
            tuple(sorted((key, _static_value(item)) for key, item in value.items()))
        )
    raise TypeError("capture static argument is not an I-JSON value")



def _contains_registered_rule(namespace: dict[str, object]) -> bool:
    def is_rule(value: object) -> bool:
        return isinstance(value, Definition) and value.kind == "rule"

    for value in namespace.values():
        if is_rule(value):
            return True
        if isinstance(value, (type, ModuleType)) and any(
            is_rule(item) for item in vars(value).values()
        ):
            return True
    return False


def _contains_registered_module(namespace: dict[str, object]) -> bool:
    def is_module(value: object) -> bool:
        return isinstance(value, Definition) and value.kind in {
            "module",
            "module_decl",
        }

    for value in namespace.values():
        if is_module(value):
            return True
        if isinstance(value, (type, ModuleType)) and any(
            is_module(item) for item in vars(value).values()
        ):
            return True
    return False


def _capture_queue_rule(
    text: str,
    system: str,
    source_path: str,
    static_arguments: dict[str, StaticValue],
    definition_ndf: DefinitionNdfMetadata | None = None,
    definition_locations: dict[str, tuple[str, int, int]] | None = None,
    source_node_locations: dict[str, tuple[SourceNodeRecord, ...]] | None = None,
) -> tuple[object, str, tuple[Diagnostic, ...]]:
    """Capture Queue/rule artifacts and preserve frontend diagnostics."""

    from ._queue_frontend import (
        QueueFrontendError,
        build_queue_acpy,
        lower_queue_source,
        parse_queue_program,
    )

    lowered = lower_queue_source(
        text,
        system,
        static_arguments=static_arguments,
        source_path=source_path,
        definition_locations=definition_locations,
        source_node_locations=source_node_locations,
        definition_ndf=definition_ndf,
    )
    try:
        diagnostics = parse_queue_program(
            text,
            system,
            static_arguments=static_arguments,
            source_path=source_path,
        ).diagnostics
    except QueueFrontendError:
        diagnostics = ()

    return (
        build_queue_acpy(text, system, source_path),
        lowered,
        diagnostics,
    )


def _flatten_source_closure(
    closure: SourceClosure,
    entry: Path,
    entry_symbol: str | None = None,
) -> tuple[
    str,
    dict[str, NdfMetadata],
    dict[str, tuple[str, int, int]],
    dict[str, tuple[SourceNodeRecord, ...]],
]:
    """Keep the entry body and only typed declarations from dependency sources.

    Each selected definition also reports the Python file that declares it, so
    the frontend owns every nominal and helper by its real source file instead
    of attributing the whole closure to the entry file. The original node
    locations travel with it as well, so the reparsed flattened text keeps the
    real file, line, and column of every statement instead of the entry file's
    flattened position.
    """

    from ._queue_compiler.provenance import extract_definition_ndf_metadata

    parsed_entries = tuple(
        (
            source_entry,
            ast.parse(
                source_entry.source,
                filename=source_entry.path,
                type_comments=True,
            ),
            Path(source_entry.source_file).resolve() == entry.resolve(),
        )
        for source_entry in closure.entries
    )

    def decorator_kinds(statement: ast.FunctionDef | ast.ClassDef) -> set[str]:
        kinds: set[str] = set()
        for decorator in statement.decorator_list:
            target = decorator.func if isinstance(decorator, ast.Call) else decorator
            if isinstance(target, ast.Attribute):
                kinds.add(target.attr)
            elif isinstance(target, ast.Name):
                kinds.add(target.id)
        return kinds

    architecture_kinds = {
        "system",
        "module",
        "module_decl",
        "extern_module",
        "process",
        "rule",
        "invariant",
    }
    closure_paths = {source_entry.path for source_entry, _, _ in parsed_entries}
    functions_by_source = {
        source_entry.path: {
            statement.name: statement
            for statement in source_tree.body
            if isinstance(statement, ast.FunctionDef)
        }
        for source_entry, source_tree, _ in parsed_entries
    }

    def imported_source(
        source_path: str, statement: ast.ImportFrom
    ) -> str | None:
        package_parts = list(PurePosixPath(source_path).parent.parts)
        if statement.level:
            ascent = statement.level - 1
            if ascent:
                package_parts = package_parts[:-ascent]
        else:
            package_parts = []
        module_parts = (
            list((statement.module or "").split("."))
            if statement.module
            else []
        )
        base = "/".join((*package_parts, *module_parts))
        for candidate in (f"{base}.py", f"{base}/__init__.py"):
            if candidate in closure_paths:
                return candidate
        return None

    imports_by_source: dict[str, dict[str, tuple[str, str]]] = {}
    for source_entry, source_tree, _ in parsed_entries:
        imports: dict[str, tuple[str, str]] = {}
        for statement in source_tree.body:
            if not isinstance(statement, ast.ImportFrom):
                continue
            target = imported_source(source_entry.path, statement)
            if target is None:
                continue
            for alias in statement.names:
                imports[alias.name] = (target, alias.name)
        imports_by_source[source_entry.path] = imports

    trees_by_source = {
        source_entry.path: source_tree
        for source_entry, source_tree, _ in parsed_entries
    }
    assignments_by_source = {
        source_path: {
            statement.targets[0].id: statement.value
            for statement in source_tree.body
            if isinstance(statement, ast.Assign)
            and len(statement.targets) == 1
            and isinstance(statement.targets[0], ast.Name)
        }
        for source_path, source_tree in trees_by_source.items()
    }
    retained_type_bindings_by_source: dict[str, set[str]] = {
        source_path: set() for source_path in trees_by_source
    }
    pending_type_bindings = [
        (source_path, node.id)
        for source_path, source_tree in trees_by_source.items()
        for statement in source_tree.body
        if isinstance(statement, ast.ClassDef)
        and (
            decorator_kinds(statement) & {"config", "struct", "bitfield"}
            or any(
                isinstance(base, ast.Name) and base.id in {"Enum", "IntEnum"}
                for base in statement.bases
            )
        )
        for node in ast.walk(statement)
        if isinstance(node, ast.Name) and isinstance(node.ctx, ast.Load)
    ]
    visited_type_bindings: set[tuple[str, str]] = set()
    while pending_type_bindings:
        source_path, name = pending_type_bindings.pop()
        key = (source_path, name)
        if key in visited_type_bindings:
            continue
        visited_type_bindings.add(key)
        expression = assignments_by_source[source_path].get(name)
        if expression is not None:
            retained_type_bindings_by_source[source_path].add(name)
            pending_type_bindings.extend(
                (source_path, node.id)
                for node in ast.walk(expression)
                if isinstance(node, ast.Name) and isinstance(node.ctx, ast.Load)
            )
            continue
        imported = imports_by_source[source_path].get(name)
        if imported is not None:
            pending_type_bindings.append(imported)

    def helper_target(source_path: str, name: str) -> tuple[str, str] | None:
        local = functions_by_source[source_path].get(name)
        if local is not None and not (decorator_kinds(local) & architecture_kinds):
            return source_path, name
        imported = imports_by_source[source_path].get(name)
        if imported is None:
            return None
        target_source, target_name = imported
        target = functions_by_source[target_source].get(target_name)
        if target is None or decorator_kinds(target) & architecture_kinds:
            return None
        return target_source, target_name

    reachable_helpers: set[tuple[str, str]] = set()
    entry_record = next(
        (
            (source_entry, source_tree)
            for source_entry, source_tree, owns_entry in parsed_entries
            if owns_entry
        ),
        None,
    )
    if entry_record is not None:
        entry_source, entry_tree = entry_record
        entry_architecture = {
            statement.name: statement
            for statement in entry_tree.body
            if isinstance(statement, ast.FunctionDef)
            and decorator_kinds(statement) & architecture_kinds
        }
        pending_architecture = (
            [entry_symbol]
            if entry_symbol is not None and entry_symbol in entry_architecture
            else list(entry_architecture)
        )
        visited_architecture: set[str] = set()
        pending_helpers: list[tuple[str, str]] = []

        def follow_names(source_path: str, function: ast.FunctionDef) -> None:
            for candidate in ast.walk(function):
                if not isinstance(candidate, ast.Name) or not isinstance(
                    candidate.ctx, ast.Load
                ):
                    continue
                if (
                    source_path == entry_source.path
                    and candidate.id in entry_architecture
                    and candidate.id not in visited_architecture
                ):
                    pending_architecture.append(candidate.id)
                    continue
                helper = helper_target(source_path, candidate.id)
                if helper is not None and helper not in reachable_helpers:
                    reachable_helpers.add(helper)
                    pending_helpers.append(helper)

        while pending_architecture:
            name = pending_architecture.pop()
            if name in visited_architecture:
                continue
            visited_architecture.add(name)
            follow_names(entry_source.path, entry_architecture[name])
        while pending_helpers:
            source_path, name = pending_helpers.pop()
            follow_names(source_path, functions_by_source[source_path][name])

        # A reachable rule may use an imported static parameter as a range or
        # array target even when no nominal declaration refers to that alias.
        # Keep its source-owned binding and its closed constant dependencies.
        pending_range_bindings: list[tuple[str, str]] = []
        reachable_functions = [
            (entry_source.path, entry_architecture[name])
            for name in visited_architecture
        ] + [
            (source_path, functions_by_source[source_path][name])
            for source_path, name in reachable_helpers
        ]
        for source_path, function in reachable_functions:
            for candidate in ast.walk(function):
                if not (
                    isinstance(candidate, ast.Subscript)
                    and isinstance(candidate.value, ast.Attribute)
                    and isinstance(candidate.value.value, ast.Name)
                    and candidate.value.value.id == "ac"
                    and candidate.value.attr in {"array", "bits", "index", "range"}
                ):
                    continue
                pending_range_bindings.extend(
                    (source_path, name.id)
                    for name in ast.walk(candidate.slice)
                    if isinstance(name, ast.Name) and isinstance(name.ctx, ast.Load)
                )
        while pending_range_bindings:
            source_path, name = pending_range_bindings.pop()
            key = (source_path, name)
            if key in visited_type_bindings:
                continue
            visited_type_bindings.add(key)
            expression = assignments_by_source[source_path].get(name)
            if expression is not None:
                retained_type_bindings_by_source[source_path].add(name)
                pending_range_bindings.extend(
                    (source_path, dependency.id)
                    for dependency in ast.walk(expression)
                    if isinstance(dependency, ast.Name)
                    and isinstance(dependency.ctx, ast.Load)
                )
                continue
            imported = imports_by_source[source_path].get(name)
            if imported is not None:
                pending_range_bindings.append(imported)

    statements: list[ast.stmt] = []
    definition_ndf: dict[str, NdfMetadata] = {}
    definition_locations: dict[str, tuple[str, int, int]] = {}
    source_node_locations: dict[str, tuple[SourceNodeRecord, ...]] = {}
    entry_owned_names: set[str] = set()
    for source_entry, source_tree, owns_entry in parsed_entries:
        selected: list[ast.stmt] = []
        retained_type_bindings = retained_type_bindings_by_source[source_entry.path]
        for statement in source_tree.body:
            if isinstance(statement, (ast.Import, ast.ImportFrom)):
                continue
            if owns_entry:
                selected.append(statement)
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and statement.targets[0].id in retained_type_bindings
            ):
                # Imported nominal declarations keep their source-owned
                # dependent type roots and closed geometry constants. Dropping
                # either leaves the class annotation in the closure but turns
                # its array/bit/range expression into an unresolved name.
                selected.append(statement)
                continue
            if isinstance(statement, ast.ClassDef) and (
                decorator_kinds(statement) & {"config", "struct", "bitfield"}
                or any(
                    isinstance(base, ast.Name)
                    and base.id in {"Enum", "IntEnum"}
                    for base in statement.bases
                )
            ):
                selected.append(statement)
                continue
            if (
                isinstance(statement, ast.FunctionDef)
                and (
                    "module_decl" in decorator_kinds(statement)
                    or (source_entry.path, statement.name) in reachable_helpers
                )
            ):
                selected.append(statement)
        statements.extend(selected)
        captured_locations = capture_source_node_locations(
            source_tree, source_entry.path
        )
        for statement in selected:
            if isinstance(statement, (ast.FunctionDef, ast.ClassDef)):
                if owns_entry or statement.name not in definition_locations:
                    definition_locations[statement.name] = (
                        source_entry.path,
                        statement.lineno,
                        statement.col_offset + 1,
                    )
                    captured = captured_locations.get(statement.name)
                    if captured is not None:
                        source_node_locations[statement.name] = captured
                if owns_entry:
                    entry_owned_names.add(statement.name)
        selected_names = {
            statement.name
            for statement in selected
            if isinstance(statement, (ast.FunctionDef, ast.ClassDef))
        }
        for name, metadata in extract_definition_ndf_metadata(
            source_entry.source
        ).items():
            if name not in selected_names:
                continue
            previous = definition_ndf.get(name)
            if previous is not None and previous != metadata:
                if not owns_entry:
                    if name in entry_owned_names:
                        continue
                    raise ValueError(
                        f"ACPY-NDF-001: definition {name!r} has ambiguous NDF metadata"
                    )
            definition_ndf[name] = metadata
    source_text = ast.unparse(
        ast.fix_missing_locations(ast.Module(statements, []))
    )
    return source_text, definition_ndf, definition_locations, source_node_locations


def _worker_main(request_path: Path) -> int:
    request = json.loads(request_path.read_text())
    workspace = Path(request["workspace"]).resolve()
    entry = Path(request["entry"]).resolve()
    output = Path(request["output"]).resolve()
    captured_stdout = io.StringIO()
    captured_stderr = io.StringIO()
    document = None
    acir = None
    frontend_kind = "structural"
    diagnostics: tuple[Diagnostic, ...]
    try:
        with (
            contextlib.redirect_stdout(captured_stdout),
            contextlib.redirect_stderr(captured_stderr),
        ):
            namespace = _load_project(entry, workspace, request["module_name"])
            component_roots = tuple(
                (workspace / value).resolve() for value in request["component_roots"]
            )
            if any(not path.is_relative_to(workspace) for path in component_roots):
                raise ValueError("component root escapes the workspace")
            text = entry.read_text(encoding="utf-8")
            has_rule = _contains_registered_rule(namespace)
            static_arguments = {
                key: _static_value(value)
                for key, value in request["static_arguments"].items()
            }
            selected = namespace.get(request["system"])
            modern_module_system = (
                isinstance(selected, Definition)
                and selected.kind == "system"
                and not dict(selected.explicit_options).get("root")
                and _contains_registered_module(namespace)
            )
            if request.get("entry_kind", "system") == "source_unit":
                frontend_kind = "queue_rule"
                source_modules = tuple(request.get("source_modules", ()))
                if len(source_modules) != 1 or type(source_modules[0]) is not str:
                    raise ValueError(
                        "source unit requires exactly one public module"
                    )
                module_name = source_modules[0]
                selected_module = namespace.get(module_name)
                if (
                    not isinstance(selected_module, Definition)
                    or selected_module.kind != "module"
                    or selected_module.source_file is None
                    or Path(selected_module.source_file).resolve() != entry
                ):
                    raise ValueError(
                        f"source-unit module {module_name!r} is not owned by "
                        f"{entry.name!r}"
                    )
                from ._queue_frontend import lower_source_unit
                from ._source_closure import capture_source_closure

                closure = capture_source_closure(entry, workspace)
                (
                    source_text,
                    definition_ndf,
                    definition_locations,
                    source_node_locations,
                ) = _flatten_source_closure(closure, entry, module_name)
                acir = lower_source_unit(
                    source_text,
                    ((module_name, ()),),
                    source_path=entry.relative_to(workspace).as_posix(),
                    definition_locations=definition_locations,
                    source_node_locations=source_node_locations,
                    definition_ndf=definition_ndf,
                )
                diagnostics = ()
            elif request["source_closure"]:
                frontend_kind = "queue_rule"
                from ._source_closure import capture_source_closure

                closure = capture_source_closure(entry, workspace)
                (
                    source_text,
                    definition_ndf,
                    definition_locations,
                    source_node_locations,
                ) = _flatten_source_closure(closure, entry, request["system"])
                document, acir, diagnostics = _capture_queue_rule(
                    source_text,
                    request["system"],
                    entry.relative_to(workspace).as_posix(),
                    static_arguments,
                    definition_ndf,
                    definition_locations,
                    source_node_locations,
                )
            elif has_rule or modern_module_system:
                frontend_kind = "queue_rule"
                document, acir, diagnostics = _capture_queue_rule(
                    text,
                    request["system"],
                    entry.relative_to(workspace).as_posix(),
                    static_arguments,
                )
            else:
                raise ValueError(
                    "legacy structural Agentic frontend is removed; use the "
                    "verified Queue/family frontend"
                )
    except BaseException as error:
        diagnostics = (_worker_diagnostic(error, entry),)
    report, _ = OutputSink.bounded_capture(
        captured_stdout.getvalue() + captured_stderr.getvalue()
    )
    (output / "model.acpy.json").write_bytes(
        document.canonical_bytes() if document is not None else b""
    )
    (output / "model.ac.mlir").write_bytes(
        acir.encode("utf-8") if acir is not None else b""
    )
    (output / "project-report.txt").write_text(report, encoding="utf-8")
    response = {
        "schema": "agentic-circuit-capture-result",
        "version": "0.1",
        "has_acpy": document is not None,
        "has_acir": acir is not None,
        "diagnostics": [item.to_json() for item in diagnostics],
        "frontend_kind": frontend_kind,
    }
    (output / "result.json").write_bytes(canonical_json_bytes(response))
    return 0


def _main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--request", type=Path, required=True)
    arguments = parser.parse_args()
    return _worker_main(arguments.request)


if __name__ == "__main__":
    raise SystemExit(_main())
