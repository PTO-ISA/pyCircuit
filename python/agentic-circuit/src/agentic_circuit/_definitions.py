"""Immutable metadata for Python architecture definitions."""

from __future__ import annotations

import inspect
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Literal, TypeAlias, TypeVar, overload

from ._families import StaticCase, StaticParameter, normalize_family_cases

DefinitionKind: TypeAlias = Literal[
    "system",
    "module",
    "module_decl",
    "extern_module",
    "struct",
    "packet",
    "transaction",
    "protocol",
    "interface",
    "process",
    "rule",
    "invariant",
    "inline",
]
F = TypeVar("F", bound=Callable[..., object])


@dataclass(frozen=True, slots=True)
class WriterPriority:
    """Compile-time priority descriptor for one Table mutation endpoint."""

    rank: int

    def __post_init__(self) -> None:
        if type(self.rank) is not int or self.rank < 0:
            raise ValueError(
                "ACPY-TABLE-011: writer priority rank must be a non-negative "
                "static integer"
            )


def writer_priority(rank: int) -> WriterPriority:
    """Declare a deterministic priority rank for a Table writer."""

    return WriterPriority(rank)


@dataclass(frozen=True, slots=True)
class Definition:
    """A captured definition registered without executing its body."""

    kind: DefinitionKind
    function: Callable[..., object]
    qualified_name: str
    explicit_options: tuple[tuple[str, object], ...]
    module_name: str
    source_file: str | None
    source_line: int | None
    annotation_types: tuple[tuple[str, type[object]], ...] = field(
        default=(), compare=False, repr=False
    )

    def __repr__(self) -> str:
        return f"Definition(kind={self.kind!r}, qualified_name={self.qualified_name!r})"

    @property
    def __signature__(self) -> inspect.Signature:
        return inspect.signature(self.function)

    @property
    def __name__(self) -> str:
        return self.function.__name__

    def __call__(self, *args: object, **kwargs: object) -> "PendingDefinitionCall":
        try:
            bound = self.__signature__.bind(*args, **kwargs)
        except TypeError as error:
            raise TypeError(f"ACPY-CALL-003: {error}") from error
        bound.apply_defaults()
        return PendingDefinitionCall(self, tuple(bound.arguments.items()))


@dataclass(frozen=True, slots=True)
class PendingDefinitionCall:
    definition: Definition
    arguments: tuple[tuple[str, object], ...]


@overload
def _decorate(definition_kind: DefinitionKind, function: F) -> Definition: ...


@overload
def _decorate(
    definition_kind: DefinitionKind, function: None = None, **options: object
) -> Callable[[F], Definition]: ...


def _decorate(
    definition_kind: DefinitionKind,
    function: F | None = None,
    *,
    _annotation_types: tuple[tuple[str, type[object]], ...] = (),
    **options: object,
) -> Definition | Callable[[F], Definition]:
    def apply(target: F) -> Definition:
        module_name = getattr(target, "__module__", "")
        try:
            source_file = inspect.getsourcefile(target)
        except (OSError, TypeError):
            # File-path loading may leave the defining module out of
            # ``sys.modules``; provenance must degrade instead of aborting.
            source_file = None
        source_line: int | None
        code = getattr(target, "__code__", None)
        if code is not None:
            source_line = code.co_firstlineno
        else:
            try:
                _, source_line = inspect.getsourcelines(target)
            except (OSError, TypeError):
                source_line = None
        return Definition(
            kind=definition_kind,
            function=target,
            qualified_name=target.__qualname__,
            explicit_options=tuple(sorted(options.items())),
            module_name=module_name,
            source_file=(
                str(Path(source_file).resolve()) if source_file is not None else None
            ),
            source_line=source_line,
            annotation_types=_annotation_types,
        )

    return apply(function) if function is not None else apply


def _visible_annotation_types() -> tuple[tuple[str, type[object]], ...]:
    frame = inspect.currentframe()
    decorator = None if frame is None else frame.f_back
    caller = None if decorator is None else decorator.f_back
    visible: dict[str, object] = {}
    if caller is not None:
        visible.update(caller.f_globals)
        visible.update(caller.f_locals)
    annotation_types = tuple(
        sorted(
            (name, value) for name, value in visible.items() if isinstance(value, type)
        )
    )
    del frame
    del decorator
    del caller
    return annotation_types


def system(function: F | None = None, **options: object):
    return _decorate(
        "system",
        function,
        _annotation_types=_visible_annotation_types(),
        **options,
    )


def module(*, declaration: Definition):
    if not isinstance(declaration, Definition) or declaration.kind != "module_decl":
        raise TypeError("ACPY-FAMILY-007: module declaration must be @module_decl")
    return _decorate(
        "module",
        _annotation_types=_visible_annotation_types(),
        declaration=declaration,
    )


def module_decl(
    *,
    source: str,
    parameters: tuple[StaticParameter, ...] = (),
    finite_cases: tuple[StaticCase, ...] | None = None,
):
    """Declare a separately compiled module without providing its body."""

    if not isinstance(source, str) or not source or source.startswith("/"):
        raise ValueError("ACPY-FAMILY-007: module declaration source must be relative")
    cases = normalize_family_cases(parameters, finite_cases)

    return _decorate(
        "module_decl",
        _annotation_types=_visible_annotation_types(),
        source=source,
        parameters=parameters,
        finite_cases=cases,
    )


def extern_module(function: F | None = None, **options: object):
    return _decorate("extern_module", function, **options)


def struct(function: F | None = None, **options: object):
    """Declare one nominal immutable payload type.

    The decorated class is replaced by a real frozen slotted dataclass.  The
    returned type keeps the captured record in ``__ac_definition__``, the
    declared ``(field, annotation)`` pairs in ``__ac_fields__``, the
    ``__ac_struct__`` marker, the ``with_fields``/``project`` replacement
    surface, and a ``descriptor`` used by runtime annotation families such as
    ``ac.array[N, Entry]``.
    """

    def apply(target: F):
        from ._structs import build_struct_class

        decorated = build_struct_class(target)
        decorated.__ac_definition__ = _decorate(  # type: ignore[attr-defined]
            "struct", target, **options
        )
        return decorated

    return apply(function) if function is not None else apply


def packet(function: F | None = None, **options: object):
    return _decorate("packet", function, **options)


def transaction(function: F | None = None, **options: object):
    return _decorate("transaction", function, **options)


def protocol(function: F | None = None, **options: object):
    return _decorate("protocol", function, **options)


def interface(function: F | None = None, **options: object):
    return _decorate("interface", function, **options)


def process(function: F | None = None, **options: object):
    return _decorate("process", function, **options)


def rule(function: F | None = None, **options: object):
    """Declare one schedulable rule without executing its Python body."""

    return _decorate("rule", function, **options)


def invariant(function: F | None = None, **options: object):
    """Declare one pure typed payload invariant."""

    return _decorate("invariant", function, **options)


def inline(function: F) -> Definition:
    """Mark one typed pure helper for mandatory ACIR inlining."""

    return _decorate("inline", function)
