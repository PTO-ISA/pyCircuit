"""Deterministic workspace-local Python import closure for JIT identity."""

from __future__ import annotations

import ast
from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

from ._canonical_json import canonical_json_bytes, sha256_bytes
from ._diagnostics import DiagnosticError


class SourceClosureError(DiagnosticError):
    """A fail-closed source-closure diagnostic."""


@dataclass(frozen=True, slots=True)
class SourceClosureEntry:
    path: str
    source_file: str
    sha256: str


@dataclass(frozen=True, slots=True)
class SourceClosure:
    entries: tuple[SourceClosureEntry, ...]
    sha256: str


class _BindingKind(Enum):
    BUILTINS = auto()
    DYNAMIC = auto()
    IMPORTLIB = auto()
    NAMESPACE_REFLECTOR = auto()
    REFLECTOR = auto()


_DYNAMIC_BUILTINS = frozenset({"__import__", "eval", "exec"})
_NAMESPACE_REFLECTORS = frozenset({"globals", "locals", "vars"})
_FORBIDDEN_DYNAMIC_MODULES = frozenset({"builtins", "importlib", "operator"})
_ALLOWED_EXTERNAL_MODULES = frozenset(
    {"__future__", "agentic_circuit", "enum"}
)
_ALLOWED_ENUM_IMPORTS = frozenset({"Enum", "IntEnum", "auto", "unique"})
_FORBIDDEN_REFLECTIVE_NAMES = frozenset(
    {
        "__dict__",
        "__getattribute__",
        "attrgetter",
        "delattr",
        "getattr",
        "setattr",
    }
)


def _bound_names(target: ast.expr) -> tuple[str, ...]:
    if isinstance(target, ast.Name):
        return (target.id,)
    if isinstance(target, (ast.List, ast.Tuple)):
        return tuple(name for element in target.elts for name in _bound_names(element))
    return ()


def _flattened_definition_names(tree: ast.Module) -> tuple[str, ...]:
    """Return definitions that enter the flattened Queue frontend namespace."""

    names: list[str] = []
    for statement in tree.body:
        if isinstance(statement, (ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)):
            names.append(statement.name)
            continue
        if isinstance(statement, ast.Assign):
            for target in statement.targets:
                names.extend(name for name in _bound_names(target) if name.isupper())
            continue
        if isinstance(statement, ast.AnnAssign):
            names.extend(
                name for name in _bound_names(statement.target) if name.isupper()
            )
    return tuple(dict.fromkeys(names))


class _DynamicCodeAnalyzer:
    """Conservatively reject bindings that can evade the static closure."""

    def __init__(self, *, source: str) -> None:
        self.source = source
        self.bindings: dict[str, set[_BindingKind]] = {
            "__builtins__": {_BindingKind.BUILTINS},
            "__import__": {_BindingKind.DYNAMIC},
            "builtins": {_BindingKind.BUILTINS},
            "eval": {_BindingKind.DYNAMIC},
            "exec": {_BindingKind.DYNAMIC},
            "getattr": {_BindingKind.REFLECTOR},
            "globals": {_BindingKind.NAMESPACE_REFLECTOR},
            "importlib": {_BindingKind.IMPORTLIB},
            "locals": {_BindingKind.NAMESPACE_REFLECTOR},
            "vars": {_BindingKind.NAMESPACE_REFLECTOR},
        }

    def _fail(self, detail: str) -> None:
        raise SourceClosureError(
            f"ACPY-JIT-006: {detail} is forbidden in {self.source}"
        )

    def _add_binding(self, name: str, *kinds: _BindingKind) -> bool:
        current = self.bindings.setdefault(name, set())
        before = len(current)
        current.update(kinds)
        return len(current) != before

    def _binding_kinds(self, expression: ast.expr) -> frozenset[_BindingKind]:
        if isinstance(expression, ast.Name):
            return frozenset(self.bindings.get(expression.id, ()))
        if not isinstance(expression, ast.Attribute):
            return frozenset()
        owner = self._binding_kinds(expression.value)
        kinds: set[_BindingKind] = set()
        if _BindingKind.BUILTINS in owner:
            if expression.attr == "getattr":
                kinds.add(_BindingKind.REFLECTOR)
            if expression.attr in _NAMESPACE_REFLECTORS:
                kinds.add(_BindingKind.NAMESPACE_REFLECTOR)
            if expression.attr in _DYNAMIC_BUILTINS:
                kinds.add(_BindingKind.DYNAMIC)
        if _BindingKind.IMPORTLIB in owner and expression.attr == "import_module":
            kinds.add(_BindingKind.DYNAMIC)
        return frozenset(kinds)

    def _record_import_bindings(self, tree: ast.AST) -> None:
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    if alias.name == "builtins":
                        self._add_binding(
                            alias.asname or "builtins", _BindingKind.BUILTINS
                        )
                    elif alias.name == "importlib":
                        self._add_binding(
                            alias.asname or "importlib", _BindingKind.IMPORTLIB
                        )
            elif isinstance(node, ast.ImportFrom) and node.level == 0:
                if node.module == "builtins":
                    for alias in node.names:
                        bound = alias.asname or alias.name
                        if alias.name in _DYNAMIC_BUILTINS:
                            self._fail(f"dynamic builtin binding {bound!r}")
                        if alias.name == "__dict__":
                            self._add_binding(bound, _BindingKind.BUILTINS)
                        if alias.name == "__getattribute__":
                            self._fail(f"reflective builtin binding {bound!r}")
                        if alias.name == "getattr" or (
                            alias.name in _NAMESPACE_REFLECTORS
                        ):
                            self._add_binding(
                                bound,
                                _BindingKind.REFLECTOR
                                if alias.name == "getattr"
                                else _BindingKind.NAMESPACE_REFLECTOR,
                            )
                elif node.module == "importlib":
                    for alias in node.names:
                        bound = alias.asname or alias.name
                        if alias.name == "import_module":
                            self._fail(f"dynamic import binding {bound!r}")
                        if alias.name == "__dict__":
                            self._add_binding(bound, _BindingKind.IMPORTLIB)
                        if alias.name == "__getattribute__":
                            self._fail(f"reflective importlib binding {bound!r}")

    def _record_assignment_aliases(self, tree: ast.AST) -> None:
        assignments: list[tuple[tuple[str, ...], ast.expr]] = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    assignments.append((_bound_names(target), node.value))
            elif isinstance(node, ast.AnnAssign) and node.value is not None:
                assignments.append((_bound_names(node.target), node.value))
            elif isinstance(node, ast.NamedExpr):
                assignments.append((_bound_names(node.target), node.value))

        changed = True
        while changed:
            changed = False
            for names, value in assignments:
                kinds = self._binding_kinds(value)
                if not kinds:
                    continue
                for name in names:
                    changed |= self._add_binding(name, *kinds)

    def _is_sensitive_namespace(self, expression: ast.expr) -> bool:
        kinds = self._binding_kinds(expression)
        if kinds & {_BindingKind.BUILTINS, _BindingKind.IMPORTLIB}:
            return True
        return (
            isinstance(expression, ast.Attribute)
            and expression.attr == "__dict__"
            and bool(
                self._binding_kinds(expression.value)
                & {_BindingKind.BUILTINS, _BindingKind.IMPORTLIB}
            )
        )

    def analyze(self, tree: ast.AST) -> None:
        for node in ast.walk(tree):
            if isinstance(node, ast.Import) and any(
                alias.name.partition(".")[0] in _FORBIDDEN_DYNAMIC_MODULES
                for alias in node.names
            ):
                self._fail("dynamic or reflective module import")
            if (
                isinstance(node, ast.ImportFrom)
                and (node.module or "").partition(".")[0] in _FORBIDDEN_DYNAMIC_MODULES
            ):
                self._fail("dynamic or reflective module import")
            if isinstance(node, ast.Attribute) and (
                node.attr in _FORBIDDEN_REFLECTIVE_NAMES or node.attr.startswith("__")
            ):
                self._fail(f"reflective attribute access {node.attr!r}")
            if isinstance(node, ast.Call):
                function_name = (
                    node.func.id
                    if isinstance(node.func, ast.Name)
                    else node.func.attr
                    if isinstance(node.func, ast.Attribute)
                    else ""
                )
                if function_name in _FORBIDDEN_REFLECTIVE_NAMES:
                    self._fail(f"reflective call {function_name!r}")
        self._record_import_bindings(tree)
        self._record_assignment_aliases(tree)

        for node in ast.walk(tree):
            if (
                isinstance(node, ast.Name)
                and isinstance(node.ctx, ast.Load)
                and _BindingKind.DYNAMIC in self._binding_kinds(node)
            ):
                self._fail(f"dynamic code or import access {node.id!r}")

            if isinstance(node, ast.Attribute):
                if _BindingKind.DYNAMIC in self._binding_kinds(node):
                    self._fail(f"dynamic code or import access {node.attr!r}")
                if node.attr == "__dict__" and self._binding_kinds(node.value) & {
                    _BindingKind.BUILTINS,
                    _BindingKind.IMPORTLIB,
                }:
                    self._fail("reflective namespace access")
                if node.attr == "__getattribute__" and self._binding_kinds(
                    node.value
                ) & {_BindingKind.BUILTINS, _BindingKind.IMPORTLIB}:
                    self._fail("reflective getattr access")

            if isinstance(node, ast.Call):
                if (
                    _BindingKind.REFLECTOR in self._binding_kinds(node.func)
                    and node.args
                    and self._is_sensitive_namespace(node.args[0])
                ):
                    self._fail("reflective getattr access")
                if _BindingKind.NAMESPACE_REFLECTOR in self._binding_kinds(node.func):
                    self._fail("reflective namespace access")

            if isinstance(node, ast.Subscript) and self._is_sensitive_namespace(
                node.value
            ):
                self._fail("reflective subscript access")


def _entry_package_initializers(entry: Path, root: Path) -> tuple[Path, ...]:
    relative = entry.relative_to(root)
    package_parts = relative.parent.parts
    return tuple(
        initializer
        for index in range(1, len(package_parts) + 1)
        if (
            initializer := root.joinpath(*package_parts[:index], "__init__.py")
        ).is_file()
    )


def _module_candidates(root: Path, parts: tuple[str, ...]) -> tuple[Path, ...]:
    if not parts:
        return ()
    candidates: list[Path] = []
    for index in range(1, len(parts)):
        package = root.joinpath(*parts[:index], "__init__.py")
        if package.is_file():
            candidates.append(package)
    module = root.joinpath(*parts).with_suffix(".py")
    package = root.joinpath(*parts, "__init__.py")
    if module.is_file():
        candidates.append(module)
    elif package.is_file():
        candidates.append(package)
    return tuple(candidates)


def _import_targets(
    node: ast.Import | ast.ImportFrom,
    *,
    source: Path,
    root: Path,
) -> tuple[Path, ...]:
    if isinstance(node, ast.Import):
        targets: list[Path] = []
        for alias in node.names:
            root_name = alias.name.partition(".")[0]
            if root_name in _ALLOWED_EXTERNAL_MODULES:
                if any(part.startswith("_") for part in alias.name.split(".")):
                    raise SourceClosureError(
                        f"ACPY-JIT-006: private external import {alias.name!r} "
                        f"is forbidden in {source.relative_to(root)}"
                    )
                continue
            local = _module_candidates(root, tuple(alias.name.split(".")))
            if not local:
                raise SourceClosureError(
                    f"ACPY-JIT-006: external import {alias.name!r} is not "
                    f"allowed in {source.relative_to(root)}"
                )
            raise SourceClosureError(
                "ACPY-JIT-006: module-qualified local import "
                f"{alias.name!r} is not supported in {source.relative_to(root)}; "
                "import explicit symbols with 'from ... import ...'"
            )
        return tuple(targets)

    if any(alias.name == "*" for alias in node.names):
        raise SourceClosureError(f"ACPY-JIT-006: star import is forbidden in {source}")
    if node.level == 0 and (node.module or "").partition(".")[0] in (
        _ALLOWED_EXTERNAL_MODULES
    ):
        if node.module == "enum" and any(
            alias.name not in _ALLOWED_ENUM_IMPORTS for alias in node.names
        ):
            raise SourceClosureError(
                "ACPY-JIT-006: enum imports are restricted to "
                f"{sorted(_ALLOWED_ENUM_IMPORTS)} in {source.relative_to(root)}"
            )
        if (node.module or "").partition(".")[0] == "agentic_circuit" and (
            any(part.startswith("_") for part in (node.module or "").split("."))
            or any(alias.name.startswith("_") for alias in node.names)
        ):
            raise SourceClosureError(
                f"ACPY-JIT-006: private external import is forbidden in "
                f"{source.relative_to(root)}"
            )
        return ()

    relative = source.relative_to(root)
    package_parts = list(relative.parent.parts)
    if node.level:
        ascent = node.level - 1
        if ascent > len(package_parts):
            raise SourceClosureError(
                f"ACPY-JIT-006: relative import escapes workspace in {relative}"
            )
        if ascent:
            package_parts = package_parts[:-ascent]
    else:
        package_parts = []
    module_parts = list((node.module or "").split(".")) if node.module else []
    base_parts = (*package_parts, *module_parts)
    targets = list(_module_candidates(root, base_parts))
    package_directory = root.joinpath(*base_parts)
    if package_directory.is_dir():
        for alias in node.names:
            imported_module = _module_candidates(root, (*base_parts, alias.name))
            if imported_module:
                raise SourceClosureError(
                    "ACPY-JIT-006: module-qualified local import "
                    f"{alias.name!r} is not supported in {relative}; import "
                    "explicit symbols from that module"
                )
    renamed = next((alias for alias in node.names if alias.asname is not None), None)
    if renamed is not None:
        raise SourceClosureError(
            "ACPY-JIT-006: renamed local import "
            f"{renamed.name!r} as {renamed.asname!r} is not supported in {relative}"
        )
    if not targets:
        kind = "external" if node.level == 0 else "unresolved relative"
        raise SourceClosureError(
            f"ACPY-JIT-006: {kind} import {node.module!r} is not allowed in {relative}"
        )
    return tuple(dict.fromkeys(targets))


def capture_source_closure(entry: Path, workspace: Path) -> SourceClosure:
    """Capture a normalized closure of workspace-local static imports."""

    root = workspace.resolve(strict=True)
    initial = entry.resolve(strict=True)
    try:
        initial.relative_to(root)
    except ValueError as error:
        raise SourceClosureError(
            f"ACPY-JIT-006: source {initial} is outside workspace {root}"
        ) from error

    pending = [initial, *reversed(_entry_package_initializers(initial, root))]
    visited: set[Path] = set()
    entries: list[SourceClosureEntry] = []
    definition_sources: dict[str, str] = {}
    while pending:
        source = pending.pop()
        resolved = source.resolve(strict=True)
        if resolved in visited:
            continue
        try:
            relative = resolved.relative_to(root).as_posix()
        except ValueError as error:
            raise SourceClosureError(
                f"ACPY-JIT-006: imported source {resolved} escapes workspace {root}"
            ) from error
        visited.add(resolved)
        raw = resolved.read_bytes()
        try:
            text = raw.decode("utf-8")
        except UnicodeDecodeError as error:
            raise SourceClosureError(
                f"ACPY-JIT-006: source {relative!r} is not UTF-8"
            ) from error
        tree = ast.parse(text, filename=relative, type_comments=True)
        _DynamicCodeAnalyzer(source=relative).analyze(tree)
        for name in _flattened_definition_names(tree):
            prior = definition_sources.get(name)
            if prior is not None and prior != relative:
                raise SourceClosureError(
                    "ACPY-JIT-006: flattened source symbol "
                    f"{name!r} is defined by both {prior!r} and {relative!r}"
                )
            definition_sources[name] = relative
        for node in ast.walk(tree):
            if isinstance(node, (ast.Import, ast.ImportFrom)):
                pending.extend(
                    reversed(_import_targets(node, source=resolved, root=root))
                )
        entries.append(
            SourceClosureEntry(
                path=relative,
                source_file=str(resolved),
                sha256=sha256_bytes(raw),
            )
        )

    ordered = tuple(sorted(entries, key=lambda item: item.path))
    fingerprint = sha256_bytes(
        canonical_json_bytes(
            [{"path": item.path, "sha256": item.sha256} for item in ordered]
        )
    )
    return SourceClosure(ordered, fingerprint)
