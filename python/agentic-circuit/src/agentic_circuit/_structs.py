"""Runtime nominal payload classes for ``@ac.struct`` declarations.

The Queue compiler resolves payload layouts from the authored source AST; it
never reads the decorated Python object.  This module owns the ordinary Python
authoring surface instead: a real nominal type with immutable keyword
construction, the compiler-visible field metadata, and a runtime descriptor
that lets ``ac.array[N, Entry]`` annotations evaluate without a postponed
annotations import.
"""

from __future__ import annotations

import dataclasses
from dataclasses import dataclass

from _pycircuit_semantics import (
    ArrayType,
    BitsType,
    BoolType,
    EnumType,
    RangeType,
    StructType,
    TupleType,
    ValueType,
)

from ._types import ArrayAnnotation, RangeAnnotation, ScalarType

# Field names occupied by the runtime surface.  The compiler never declares a
# field with one of these names, so rejecting them keeps the payload class and
# its metadata unambiguous.
_RESERVED_FIELD_NAMES = frozenset(
    {
        "__ac_definition__",
        "__ac_fields__",
        "__ac_struct__",
        "descriptor",
        "project",
        "with_fields",
    }
)


@dataclass(frozen=True, slots=True)
class StructAnnotation(ValueType):
    """Runtime nominal descriptor for one ``@ac.struct`` payload class.

    The compiler owns the concrete layout and resolves it from source.  This
    descriptor records the declaration name and the declared ``(field, type)``
    pairs in order so struct classes participate in runtime annotation families
    such as ``ac.array[N, Entry]``.  ``bit_width()`` is exact whenever every
    declared field has a static width and raises otherwise.
    """

    name: str
    fields: tuple[tuple[str, object], ...]

    def bit_width(self) -> int:
        total = 0
        for _, declared in self.fields:
            total += _declared_bit_width(declared)
        return total


def _declared_bit_width(declared: object) -> int:
    if declared is bool:
        return 1
    if isinstance(declared, ScalarType):
        if type(declared.width) is int:
            return declared.width
        raise TypeError(
            "ACPY-TYPE-006: struct field width must be static for this descriptor"
        )
    if isinstance(declared, BoolType | BitsType | EnumType | RangeType | TupleType):
        return declared.bit_width()
    if isinstance(declared, ArrayType | StructType):
        return declared.bit_width()
    if isinstance(declared, ArrayAnnotation):
        if type(declared.length) is int:
            return declared.length * _declared_bit_width(declared.element)
        raise TypeError(
            "ACPY-TYPE-006: struct field array length must be static for this "
            "descriptor"
        )
    if isinstance(declared, RangeAnnotation):
        if type(declared.lower) is int and type(declared.upper) is int:
            return max(1, (declared.upper - 1).bit_length())
        raise TypeError(
            "ACPY-TYPE-006: struct field range bounds must be static for this "
            "descriptor"
        )
    captured = getattr(declared, "descriptor", None)
    if isinstance(captured, StructAnnotation):
        return captured.bit_width()
    raise TypeError(
        f"ACPY-TYPE-006: struct field type {declared!r} has no runtime width"
    )


def _with_fields(self: object, **fields: object) -> object:
    """Return an immutable copy with the named fields replaced."""

    return dataclasses.replace(self, **fields)


def _project(self: object, target: object) -> object:
    """Return one smaller nominal record with the target's declared fields."""

    fields = getattr(target, "__ac_fields__", None)
    if fields is None:
        raise TypeError(
            "ACPY-TYPE-006: record project target must be an @ac.struct class"
        )
    values: dict[str, object] = {}
    for name, _ in fields:
        try:
            values[name] = getattr(self, name)
        except AttributeError as error:
            raise TypeError(
                f"ACPY-TYPE-006: record project source is missing field {name!r}"
            ) from error
    return target(**values)


def build_struct_class(cls: type[object]) -> type[object]:
    """Build the real nominal class returned by ``@ac.struct``."""

    if not isinstance(cls, type):
        raise TypeError("ACPY-QUEUE-002: @ac.struct must decorate a class")
    if cls.__bases__ != (object,):
        raise TypeError("ACPY-QUEUE-002: struct inheritance is not supported")
    if dataclasses.is_dataclass(cls):
        raise TypeError(
            "ACPY-QUEUE-002: struct class must not already be a dataclass"
        )
    annotations = dict(getattr(cls, "__annotations__", {}))
    if not annotations:
        raise TypeError("ACPY-QUEUE-002: struct requires annotated fields")
    reserved = sorted(_RESERVED_FIELD_NAMES.intersection(annotations))
    if reserved:
        raise TypeError(
            f"ACPY-QUEUE-002: struct field name {reserved[0]!r} is reserved"
        )

    cls.with_fields = _with_fields  # type: ignore[attr-defined]
    cls.project = _project  # type: ignore[attr-defined]
    frozen = dataclasses.dataclass(frozen=True, slots=True, kw_only=True)(cls)
    fields = tuple(annotations.items())
    frozen.__ac_struct__ = True  # type: ignore[attr-defined]
    frozen.__ac_fields__ = fields  # type: ignore[attr-defined]
    frozen.descriptor = StructAnnotation(cls.__name__, fields)  # type: ignore[attr-defined]
    return frozen
