"""Closed configuration records and deterministic JIT specialization metadata."""

from __future__ import annotations

import ast
import dataclasses
import enum
import inspect
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from types import MappingProxyType
from typing import ForwardRef, get_args, get_origin, get_type_hints

from ._canonical_json import validate_ijson_value
from ._definitions import Definition
from ._diagnostics import Diagnostic, DiagnosticRuntimeError, DiagnosticTypeError
from ._package_data import repository_root
from ._source_closure import SourceClosureEntry, capture_source_closure
from ._source_map import SourceNodeLocations, capture_source_node_locations
from ._static_eval import FrozenMap, StaticValue, static_json_value
from ._types import Static


def config(cls: type[object]) -> type[object]:
    """Freeze one closed elaboration-time configuration record."""

    if not isinstance(cls, type):
        raise DiagnosticTypeError("ACPY-JIT-001: config must decorate a class")
    if dataclasses.is_dataclass(cls):
        raise DiagnosticTypeError(
            "ACPY-JIT-001: config class must not already be a dataclass"
        )
    frame = inspect.currentframe()
    caller = None if frame is None else frame.f_back
    module = sys.modules.get(cls.__module__)
    try:
        field_types = get_type_hints(
            cls,
            globalns={} if module is None else vars(module),
            localns={} if caller is None else dict(caller.f_locals),
        )
    except (NameError, TypeError):
        field_types = dict(getattr(cls, "__annotations__", {}))
    finally:
        del frame
        del caller
    frozen = dataclasses.dataclass(frozen=True, slots=True)(cls)
    frozen.__ac_config__ = True  # type: ignore[attr-defined]
    frozen.__ac_config_field_types__ = MappingProxyType(  # type: ignore[attr-defined]
        dict(field_types)
    )
    return frozen


def _is_const_annotation(annotation: object) -> bool:
    if get_origin(annotation) is Static:
        return True
    if isinstance(annotation, str):
        compact = annotation.replace(" ", "")
        return compact.startswith(("const[", "ac.const[", "Static[", "ac.Static["))
    return False


def _const_annotation_target(annotation: object) -> object | str | None:
    if get_origin(annotation) is Static:
        arguments = get_args(annotation)
        if len(arguments) != 1:
            return None
        target = arguments[0]
        return target.__forward_arg__ if isinstance(target, ForwardRef) else target
    if not isinstance(annotation, str):
        return None
    compact = annotation.replace(" ", "")
    prefixes = ("const[", "ac.const[", "Static[", "ac.Static[")
    prefix = next((item for item in prefixes if compact.startswith(item)), None)
    if prefix is None or not compact.endswith("]"):
        return None
    target = compact[len(prefix) : -1]
    try:
        literal = ast.literal_eval(target)
    except (SyntaxError, ValueError):
        return target
    return literal if isinstance(literal, str) and literal else target


def _validate_config_instance(
    value: object,
    expected: type[object],
    *,
    path: str,
    active: set[int] | None = None,
) -> None:
    if type(value) is not expected:
        raise DiagnosticTypeError(f"ACPY-JIT-002: {path} requires {expected.__name__}")
    active = set() if active is None else active
    identity = id(value)
    if identity in active:
        raise DiagnosticTypeError(
            f"ACPY-JIT-002: cyclic config value at {path} is unsupported"
        )
    active.add(identity)
    from ._types import _config_field_types

    for field_name, field_type in _config_field_types(expected).items():
        field_value = getattr(value, field_name)
        if isinstance(field_type, str):
            primitive = {
                "int": int,
                "bool": bool,
                "float": float,
                "str": str,
            }.get(field_type)
            if primitive is None:
                raise DiagnosticTypeError(
                    "ACPY-JIT-002: unresolved config field annotation "
                    f"{field_type!r} at {path}.{field_name}"
                )
            field_type = primitive
        if isinstance(field_type, type) and getattr(field_type, "__ac_config__", False):
            _validate_config_instance(
                field_value,
                field_type,
                path=f"{path}.{field_name}",
                active=active,
            )
            continue
        if (
            field_type in {int, bool, float, str}
            and type(field_value) is not field_type
        ):
            raise DiagnosticTypeError(
                f"ACPY-JIT-002: {path}.{field_name} requires {field_type.__name__}"
            )
    active.remove(identity)


def _validate_const_argument(
    parameter: inspect.Parameter,
    value: object,
    system: Definition,
) -> None:
    target = _const_annotation_target(parameter.annotation)
    if isinstance(target, type) and getattr(target, "__ac_config__", False):
        if type(value) is not target:
            raise DiagnosticTypeError(
                "ACPY-JIT-002: const argument "
                f"{parameter.name!r} requires config type {target.__name__!r}"
            )
        _validate_config_instance(value, target, path=target.__name__)
        return
    if not isinstance(target, str):
        return
    expected_name = target.rsplit(".", 1)[-1]
    annotation_types = dict(system.annotation_types)
    expected_type = annotation_types.get(target) or annotation_types.get(expected_name)
    if expected_type is None:
        expected_type = {
            "bool": bool,
            "float": float,
            "int": int,
            "object": object,
            "str": str,
        }.get(target)
    if expected_type is None:
        raise DiagnosticTypeError(
            "ACPY-JIT-002: const argument "
            f"{parameter.name!r} has unresolved annotation {target!r}"
        )
    if getattr(expected_type, "__ac_config__", False):
        expected_config = expected_type
        if type(value) is not expected_config:
            raise DiagnosticTypeError(
                "ACPY-JIT-002: const argument "
                f"{parameter.name!r} requires config type {expected_name!r}"
            )
        _validate_config_instance(value, expected_config, path=expected_name)
        return
    if getattr(type(value), "__ac_config__", False):
        raise DiagnosticTypeError(
            "ACPY-JIT-002: const argument "
            f"{parameter.name!r} annotation {target!r} is not an @ac.config type"
        )


def _closed(value: object) -> StaticValue:
    if value is None or type(value) in {bool, int, float, str}:
        result = value
    elif isinstance(value, enum.Enum):
        result = _closed(value.value)
    elif dataclasses.is_dataclass(value) and not isinstance(value, type):
        result = FrozenMap(
            tuple(
                sorted(
                    (
                        field.name,
                        _closed(getattr(value, field.name)),
                    )
                    for field in dataclasses.fields(value)
                )
            )
        )
    elif type(value) in {tuple, list}:
        result = tuple(_closed(item) for item in value)
    elif type(value) is dict:
        if any(type(key) is not str or not key for key in value):
            raise DiagnosticTypeError(
                "ACPY-JIT-002: const map keys must be non-empty strings"
            )
        result = FrozenMap(
            tuple(sorted((key, _closed(item)) for key, item in value.items()))
        )
    else:
        raise DiagnosticTypeError(
            f"ACPY-JIT-002: unsupported const value {type(value).__name__}"
        )
    try:
        validate_ijson_value(static_json_value(result))
    except ValueError as error:
        raise DiagnosticTypeError(f"ACPY-JIT-002: {error}") from error
    return result


def _display(value: StaticValue):
    if isinstance(value, FrozenMap):
        return tuple((key, _display(item)) for key, item in value.entries)
    if isinstance(value, tuple):
        return tuple(_display(item) for item in value)
    return value


@dataclass(frozen=True, slots=True)
class JitSpecialization:
    definition: Definition
    arguments: tuple[tuple[str, StaticValue], ...]
    workspace: str | None
    root_system_identity: str
    sources: tuple[SourceClosureEntry, ...]
    diagnostics: tuple[Diagnostic, ...] = ()

    @property
    def canonical_arguments(self) -> tuple[tuple[str, object], ...]:
        return tuple((name, _display(value)) for name, value in self.arguments)

    def __repr__(self) -> str:
        return f"JitSpecialization(system={self.root_system_identity!r})"

    def _captured_sources(self) -> tuple[SourceClosureEntry, ...]:
        return self.sources

    def _source(self) -> str:
        entries = self._captured_sources()
        if entries:
            if len(entries) == 1:
                return entries[0].source
            statements: list[ast.stmt] = []
            for entry in entries:
                tree = ast.parse(entry.source, filename=entry.path, type_comments=True)
                statements.extend(
                    statement
                    for statement in tree.body
                    if not isinstance(statement, (ast.Import, ast.ImportFrom))
                )
            return ast.unparse(ast.fix_missing_locations(ast.Module(statements, [])))
        if self.definition.source_file is None:
            raise DiagnosticRuntimeError(
                "ACPY-JIT-003: system has no readable source file"
            )
        path = Path(self.definition.source_file)
        if not path.is_file():
            raise DiagnosticRuntimeError(
                "ACPY-JIT-003: system source file is unavailable"
            )
        return path.read_text(encoding="utf-8")

    def _display_source_path(self) -> str | None:
        """Return a stable project-relative path without exposing host paths."""

        if self.definition.source_file is None:
            return None
        source = Path(self.definition.source_file).resolve()
        roots: list[Path] = []
        if self.workspace is not None:
            roots.append(Path(self.workspace).resolve())
        try:
            roots.append(repository_root().resolve())
        except FileNotFoundError:
            pass
        roots.append(Path.cwd().resolve())
        for root in roots:
            try:
                return source.relative_to(root).as_posix()
            except ValueError:
                continue
        return source.name

    def _definition_locations(self) -> dict[str, tuple[str, int, int]]:
        """Retain original definition locations across source-closure merging."""

        entries = self._captured_sources()
        if not entries:
            return {}
        locations: dict[str, tuple[str, int, int]] = {}
        for entry in entries:
            tree = ast.parse(entry.source, filename=entry.path, type_comments=True)
            for node in tree.body:
                if isinstance(
                    node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)
                ):
                    locations.setdefault(
                        node.name, (entry.path, node.lineno, node.col_offset + 1)
                    )
        return locations

    def _definition_ndf_metadata(self):
        """Capture adjacent NDF comments before closure flattening drops comments."""

        from ._queue_compiler.provenance import extract_definition_ndf_metadata

        sources: list[tuple[str, str]] = []
        if self.sources:
            sources.extend((entry.path, entry.source) for entry in self.sources)
        elif self.definition.source_file is not None:
            path = Path(self.definition.source_file)
            sources.append((path.name, path.read_text(encoding="utf-8")))
        result = {}
        for _source_path, source_text in sources:
            for name, metadata in extract_definition_ndf_metadata(source_text).items():
                previous = result.get(name)
                if previous is not None and previous != metadata:
                    raise DiagnosticRuntimeError(
                        f"ACPY-NDF-001: definition {name!r} has ambiguous NDF metadata"
                    )
                result[name] = metadata
        return result

    def _static_assert_locations(
        self,
    ) -> dict[str, tuple[tuple[str, int, int], ...]]:
        """Capture direct assertion spans before source-closure normalization."""

        entries = self._captured_sources()
        if not entries:
            return {}
        locations: dict[str, tuple[tuple[str, int, int], ...]] = {}
        for entry in entries:
            tree = ast.parse(entry.source, filename=entry.path, type_comments=True)
            for node in tree.body:
                if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    continue
                assertions: list[tuple[str, int, int]] = []
                for statement in node.body:
                    call = (
                        statement.value
                        if isinstance(statement, ast.Expr)
                        and isinstance(statement.value, ast.Call)
                        else None
                    )
                    name = (
                        call.func.id
                        if call is not None and isinstance(call.func, ast.Name)
                        else call.func.attr
                        if call is not None and isinstance(call.func, ast.Attribute)
                        else ""
                    )
                    if name == "static_assert":
                        assertions.append(
                            (entry.path, statement.lineno, statement.col_offset + 1)
                        )
                if assertions:
                    locations[node.name] = tuple(assertions)
        return locations

    def _source_node_locations(self) -> SourceNodeLocations:
        """Capture every original AST node before closure flattening/unparse."""

        entries = self._captured_sources()
        if not entries:
            return {}
        locations: dict[str, tuple[tuple[str, object], ...]] = {}
        for entry in entries:
            tree = ast.parse(entry.source, filename=entry.path, type_comments=True)
            for name, records in capture_source_node_locations(
                tree, entry.path
            ).items():
                if name in locations:
                    raise DiagnosticRuntimeError(
                        f"ACPY-JIT-003: source-map definition {name!r} is ambiguous"
                    )
                locations[name] = records
        return locations  # type: ignore[return-value]

    def lower_acir(self) -> str:
        """Materialize the specialization as Queue/Var ACIR text."""

        from ._queue_frontend import lower_queue_source

        return lower_queue_source(
            self._source(),
            self.definition.__name__,
            static_arguments=dict(self.arguments),
            source_path=self._display_source_path(),
            definition_locations=self._definition_locations(),
            static_assert_locations=self._static_assert_locations(),
            source_node_locations=self._source_node_locations(),
            definition_ndf=self._definition_ndf_metadata(),
        )


def jit(
    system: Definition,
    /,
    *,
    workspace: str | Path | None = None,
    **constants: object,
) -> JitSpecialization:
    """Create one deterministic system specialization.

    This captures metadata only.  It deliberately does not execute the system
    body or compile a backend artifact. Only ``ac.const`` parameters are bound
    here; ordinary typed parameters remain runtime values whose Queue
    boundaries are inferred by downstream lowering.
    """

    if not isinstance(system, Definition) or system.kind != "system":
        raise DiagnosticTypeError("ACPY-JIT-001: jit requires an @ac.system definition")
    signature = inspect.signature(system.function)
    if "workspace" in signature.parameters:
        raise DiagnosticTypeError(
            "ACPY-JIT-001: system parameter 'workspace' is reserved by jit"
        )
    parameters = tuple(signature.parameters.values())
    static_parameters = tuple(
        parameter
        for parameter in parameters
        if _is_const_annotation(parameter.annotation)
    )
    static_names = {parameter.name for parameter in static_parameters}
    runtime_names = {
        parameter.name for parameter in parameters if parameter.name not in static_names
    }
    supplied_names = set(constants)
    supplied_runtime = sorted(supplied_names & runtime_names)
    if supplied_runtime:
        raise DiagnosticTypeError(
            "ACPY-JIT-001: runtime system parameter "
            f"{supplied_runtime[0]!r} cannot be specialized"
        )
    unknown = sorted(supplied_names - static_names)
    if unknown:
        raise DiagnosticTypeError(
            f"ACPY-JIT-001: unexpected const argument {unknown[0]!r}"
        )

    arguments: list[tuple[str, StaticValue]] = []
    for parameter in static_parameters:
        if parameter.name in constants:
            value = constants[parameter.name]
        elif parameter.default is not inspect.Parameter.empty:
            value = parameter.default
        else:
            raise DiagnosticTypeError(
                f"ACPY-JIT-001: missing required const argument {parameter.name!r}"
            )
        _validate_const_argument(parameter, value, system)
        arguments.append((parameter.name, _closed(value)))
    frozen_arguments = tuple(arguments)
    sources: tuple[SourceClosureEntry, ...] = ()
    workspace_value: str | None = None
    root_system_identity = system.qualified_name
    source_file = system.source_file
    if source_file is not None:
        path = Path(source_file)
        if path.is_file():
            if workspace is not None:
                workspace_path = Path(workspace).expanduser().resolve(strict=True)
                closure = capture_source_closure(path, workspace_path)
                root = path.resolve(strict=True).relative_to(workspace_path)
                root_module = PurePosixPath(root.as_posix()).with_suffix("").as_posix()
                root_system_identity = f"{root_module}::{system.__name__}"
                workspace_value = str(workspace_path)
                sources = closure.entries
            else:
                resolved = path.resolve(strict=True)
                sources = (
                    SourceClosureEntry(
                        path=resolved.name,
                        source_file=str(resolved),
                        source=resolved.read_text(encoding="utf-8"),
                    ),
                )
    return JitSpecialization(
        system,
        frozen_arguments,
        workspace_value,
        root_system_identity,
        sources,
    )
