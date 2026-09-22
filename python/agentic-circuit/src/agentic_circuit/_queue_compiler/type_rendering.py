"""Lowest-level semantic type rendering for Queue ACIR text."""

from __future__ import annotations

from collections.abc import Iterable

from _pycircuit_semantics import (
    ArrayType,
    EnumType,
    StructType,
    TupleType,
    ValueType,
)


def _render_type(value_type: ValueType) -> str:
    """Render one semantic value type only at the ACIR text boundary."""

    return value_type.mlir()


def _nominal_declarations(value_types: Iterable[ValueType]) -> tuple[str, ...]:
    """Nominal declarations needed to type the given recursive value types.

    An ``ac.module`` family schema carries the inventory every case signature
    can be materialized from, so a nominal payload contributes its own
    declaration and, recursively, the declarations of its field types.
    Declaration order follows traversal order and repeats are dropped, which
    keeps the emitted schema deterministic.
    """

    names: list[str] = []

    def collect(value_type: ValueType) -> None:
        if isinstance(value_type, StructType):
            if value_type.name not in names:
                names.append(value_type.name)
            for field in value_type.fields:
                collect(field.type)
        elif isinstance(value_type, EnumType):
            if value_type.name not in names:
                names.append(value_type.name)
        elif isinstance(value_type, ArrayType):
            collect(value_type.element)
        elif isinstance(value_type, TupleType):
            for element in value_type.elements:
                collect(element)

    for value_type in value_types:
        collect(value_type)
    return tuple(names)
