"""Immutable recursive value-type descriptors shared by pyCircuit frontends."""

from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from typing import TypeAlias


class ValueTypeError(ValueError):
    """A deterministic rejection of an invalid value-type descriptor."""


def _name(value: object, kind: str) -> str:
    if type(value) is not str or not value.strip():
        raise ValueTypeError(f"{kind} name must be a non-empty string")
    return value.strip()


class ValueType:
    """Base contract for immutable recursive types."""

    __slots__ = ()

    def canonical(self) -> dict[str, object]:
        raise NotImplementedError

    def mlir(self, *, scope: str = "types") -> str:
        raise NotImplementedError

    def bit_width(self) -> int:
        raise NotImplementedError

    @property
    def fingerprint(self) -> str:
        encoded = json.dumps(
            self.canonical(),
            ensure_ascii=False,
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
        return "sha256:" + hashlib.sha256(encoded).hexdigest()


@dataclass(frozen=True, slots=True)
class BoolType(ValueType):
    """A logical Boolean, currently serialized as MLIR ``i1``."""

    def canonical(self) -> dict[str, object]:
        return {"kind": "bool", "version": 1}

    def mlir(self, *, scope: str = "types") -> str:
        _ = scope
        return "i1"

    def bit_width(self) -> int:
        return 1


@dataclass(frozen=True, slots=True)
class BitsType(ValueType):
    """An exact-width unsigned bit-vector."""

    width: int

    def __post_init__(self) -> None:
        if type(self.width) is not int or not 1 <= self.width <= 64:
            raise ValueTypeError("bits width must be in [1, 64]")

    def canonical(self) -> dict[str, object]:
        return {"kind": "bits", "version": 1, "width": self.width}

    def mlir(self, *, scope: str = "types") -> str:
        _ = scope
        return f"i{self.width}"

    def bit_width(self) -> int:
        return self.width


@dataclass(frozen=True, slots=True)
class EnumType(ValueType):
    """A nominal enum with stable declaration-order encoding."""

    name: str
    enumerants: tuple[str, ...]
    values: tuple[int, ...] | None = None
    declared_width: int | None = None

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _name(self.name, "enum"))
        values = tuple(_name(value, "enumerant") for value in self.enumerants)
        if not values:
            raise ValueTypeError("enum requires at least one enumerant")
        if len(set(values)) != len(values):
            raise ValueTypeError("enum enumerants must be unique")
        object.__setattr__(self, "enumerants", values)
        if (self.values is None) != (self.declared_width is None):
            raise ValueTypeError(
                "explicit enum values and declared width must be provided together"
            )
        if self.values is not None:
            encodings = tuple(self.values)
            if (
                len(encodings) != len(values)
                or any(type(value) is not int or value < 0 for value in encodings)
                or len(set(encodings)) != len(encodings)
            ):
                raise ValueTypeError(
                    "explicit enum values must be unique nonnegative integers"
                )
            if (
                type(self.declared_width) is not int
                or not 1 <= self.declared_width <= 64
                or any(value >= (1 << self.declared_width) for value in encodings)
            ):
                raise ValueTypeError(
                    "explicit enum values must fit the declared width in [1, 64]"
                )
            object.__setattr__(self, "values", encodings)

    @property
    def encoding_width(self) -> int:
        if self.declared_width is not None:
            return self.declared_width
        return max(1, (len(self.enumerants) - 1).bit_length())

    @property
    def encoding_values(self) -> tuple[int, ...]:
        return (
            tuple(range(len(self.enumerants))) if self.values is None else self.values
        )

    def encoding(self, enumerant: str) -> int:
        try:
            index = self.enumerants.index(enumerant)
        except ValueError as error:
            raise KeyError(f"unknown enumerant {enumerant!r}") from error
        return self.encoding_values[index]

    def canonical(self) -> dict[str, object]:
        result = {
            "kind": "enum",
            "version": 1,
            "name": self.name,
            "enumerants": list(self.enumerants),
            "encoding_width": self.encoding_width,
        }
        if self.values is not None:
            result["values"] = list(self.values)
            result["declared_width"] = self.declared_width
        return result

    def mlir(self, *, scope: str = "types") -> str:
        return f"!ac.enum<@{_name(scope, 'type scope')}::@{self.name}>"

    def bit_width(self) -> int:
        return self.encoding_width


@dataclass(frozen=True, slots=True)
class ValueField:
    name: str
    type: ValueType

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _name(self.name, "field"))
        if not isinstance(self.type, ValueType):
            raise TypeError("field type must be a ValueType")

    def canonical(self) -> dict[str, object]:
        return {"name": self.name, "type": self.type.canonical()}


@dataclass(frozen=True, slots=True)
class StructType(ValueType):
    """A nominal, ordered recursive struct value."""

    name: str
    fields: tuple[ValueField, ...]
    static_bindings: tuple[tuple[str, int], ...] = ()

    def __post_init__(self) -> None:
        object.__setattr__(self, "name", _name(self.name, "struct"))
        fields = tuple(self.fields)
        if not fields:
            raise ValueTypeError("struct requires at least one field")
        if not all(isinstance(field, ValueField) for field in fields):
            raise TypeError("struct fields must be ValueField values")
        if len({field.name for field in fields}) != len(fields):
            raise ValueTypeError("struct field names must be unique")
        object.__setattr__(self, "fields", fields)
        bindings = tuple(self.static_bindings)
        if any(
            type(name) is not str
            or not name
            or type(value) is not int
            or not -(1 << 63) <= value <= (1 << 63) - 1
            for name, value in bindings
        ) or len({name for name, _ in bindings}) != len(bindings):
            raise ValueTypeError(
                "struct static bindings require unique names and signed i64 values"
            )
        object.__setattr__(self, "static_bindings", tuple(sorted(bindings)))

    def field(self, name: str) -> ValueField:
        for field in self.fields:
            if field.name == name:
                return field
        raise KeyError(f"unknown field {name!r} in struct {self.name!r}")

    def canonical(self) -> dict[str, object]:
        result = {
            "kind": "struct",
            "version": 1,
            "name": self.name,
            "fields": [field.canonical() for field in self.fields],
        }
        if self.static_bindings:
            result["static_bindings"] = [
                {"name": name, "value": value} for name, value in self.static_bindings
            ]
        return result

    @property
    def specialization_fingerprint(self) -> str:
        """Return a verifier-reproducible identity for the concrete layout."""

        digest = hashlib.sha256()

        def append(value: str) -> None:
            digest.update(value.encode("utf-8"))
            digest.update(b"\0")

        append("ac.struct-specialization-v1")
        append(self.name)
        for field in self.fields:
            append(field.name)
            append(field.type.mlir())
        for name, value in self.static_bindings:
            append(name)
            append(str(value))
        return "sha256:" + digest.hexdigest()

    @property
    def symbol(self) -> str:
        """Return the stable ACIR symbol for this nominal specialization."""

        def contains_specialization(value_type: ValueType) -> bool:
            if isinstance(value_type, StructType):
                return bool(value_type.static_bindings) or any(
                    contains_specialization(field.type) for field in value_type.fields
                )
            if isinstance(value_type, TupleType):
                return any(
                    contains_specialization(item) for item in value_type.elements
                )
            if isinstance(value_type, ArrayType):
                return contains_specialization(value_type.element)
            return False

        if not self.static_bindings and not any(
            contains_specialization(field.type) for field in self.fields
        ):
            return self.name
        return f"{self.name}__p{self.specialization_fingerprint[7:19]}"

    def mlir(self, *, scope: str = "types") -> str:
        return f"!ac.struct<@{_name(scope, 'type scope')}::@{self.symbol}>"

    def bit_width(self) -> int:
        return sum(field.type.bit_width() for field in self.fields)


@dataclass(frozen=True, slots=True)
class TupleType(ValueType):
    """A structural immutable tuple value."""

    elements: tuple[ValueType, ...]

    def __post_init__(self) -> None:
        elements = tuple(self.elements)
        if not elements:
            raise ValueTypeError("tuple requires at least one element")
        if not all(isinstance(element, ValueType) for element in elements):
            raise TypeError("tuple elements must be ValueType values")
        object.__setattr__(self, "elements", elements)

    def canonical(self) -> dict[str, object]:
        return {
            "kind": "tuple",
            "version": 1,
            "elements": [element.canonical() for element in self.elements],
        }

    def mlir(self, *, scope: str = "types") -> str:
        return (
            "tuple<"
            + ", ".join(element.mlir(scope=scope) for element in self.elements)
            + ">"
        )

    def bit_width(self) -> int:
        return sum(element.bit_width() for element in self.elements)


@dataclass(frozen=True, slots=True)
class ArrayType(ValueType):
    """A structural fixed-length value array, distinct from persistent lists."""

    length: int
    element: ValueType

    def __post_init__(self) -> None:
        if type(self.length) is not int or self.length <= 0:
            raise ValueTypeError("array length must be a positive integer")
        if not isinstance(self.element, ValueType):
            raise TypeError("array element must be a ValueType")

    def canonical(self) -> dict[str, object]:
        return {
            "kind": "array",
            "version": 1,
            "length": self.length,
            "element": self.element.canonical(),
        }

    def mlir(self, *, scope: str = "types") -> str:
        return f"!ac.value_array<{self.length} x {self.element.mlir(scope=scope)}>"

    def bit_width(self) -> int:
        return self.length * self.element.bit_width()


ACType: TypeAlias = BoolType | BitsType | EnumType | StructType | TupleType | ArrayType
