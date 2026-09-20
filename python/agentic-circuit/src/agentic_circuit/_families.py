"""Typed finite-family declarations for source-owned Agentic modules."""

from __future__ import annotations

import dataclasses
import inspect
import sys
from dataclasses import dataclass
from enum import Enum
from types import MappingProxyType
from typing import Final, get_type_hints


def config(cls: type[object]) -> type[object]:
    """Freeze one closed typed configuration record."""

    if not isinstance(cls, type):
        raise TypeError("ACPY-FAMILY-001: config must decorate a class")
    if cls.__bases__ != (object,):
        raise TypeError("ACPY-FAMILY-001: config inheritance is not supported")
    if dataclasses.is_dataclass(cls):
        raise TypeError(
            "ACPY-FAMILY-001: config class must not already be a dataclass"
        )
    annotations = dict(getattr(cls, "__annotations__", {}))
    if not annotations:
        raise TypeError("ACPY-FAMILY-001: config requires annotated fields")
    admitted_namespace = {
        "__annotations__",
        "__doc__",
        "__dict__",
        "__firstlineno__",
        "__module__",
        "__qualname__",
        "__static_attributes__",
        "__type_params__",
        "__weakref__",
    }
    extra_members = tuple(
        name for name in cls.__dict__ if name not in admitted_namespace
    )
    if extra_members:
        raise TypeError(
            "ACPY-FAMILY-001: config classes admit only annotation-only fields"
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
        field_types = annotations
    finally:
        del frame
        del caller

    def validate_field_type(field_name: str, field_type: object) -> None:
        if field_type is bool:
            return
        if isinstance(field_type, StaticIntType):
            return
        if isinstance(field_type, type) and issubclass(field_type, Enum):
            if not tuple(field_type):
                raise TypeError(
                    f"ACPY-FAMILY-001: config field {field_name!r} uses an empty enum"
                )
            return
        if isinstance(field_type, type) and getattr(
            field_type, "compiler_config__", False
        ):
            return
        if field_type is int:
            raise TypeError(
                f"ACPY-FAMILY-001: config field {field_name!r} requires "
                "ac.static_int(width=..., signed=...)"
            )
        raise TypeError(
            f"ACPY-FAMILY-001: config field {field_name!r} has an unsupported "
            "open type"
        )

    for field_name, field_type in field_types.items():
        if not isinstance(field_name, str) or not field_name.isidentifier():
            raise TypeError("ACPY-FAMILY-001: config field names must be identifiers")
        validate_field_type(field_name, field_type)

    def validate_instance(self: object) -> None:
        for field_name, field_type in field_types.items():
            value = getattr(self, field_name)
            if field_type is bool:
                if type(value) is not bool:
                    raise TypeError(
                        f"ACPY-FAMILY-003: config field {field_name!r} "
                        "requires bool"
                    )
            elif isinstance(field_type, StaticIntType):
                if type(value) is not int or not _fits_integer(value, field_type):
                    raise ValueError(
                        f"ACPY-FAMILY-003: config field {field_name!r} integer "
                        "value is out of range"
                    )
            elif isinstance(field_type, type) and issubclass(field_type, Enum):
                if type(value) is not field_type:
                    raise TypeError(
                        f"ACPY-FAMILY-003: config field {field_name!r} has the "
                        "wrong enum type"
                    )
            elif type(value) is not field_type:
                raise TypeError(
                    f"ACPY-FAMILY-003: config field {field_name!r} has the "
                    "wrong config type"
                )

    cls.__post_init__ = validate_instance  # type: ignore[attr-defined]
    frozen = dataclasses.dataclass(frozen=True, slots=True)(cls)
    frozen.compiler_config__ = True  # type: ignore[attr-defined]
    frozen.compiler_config_field_types__ = MappingProxyType(  # type: ignore[attr-defined]
        dict(field_types)
    )
    return frozen


class _Missing:
    __slots__ = ()


_MISSING: Final = _Missing()


@dataclass(frozen=True, slots=True)
class StaticBoolType:
    pass


@dataclass(frozen=True, slots=True)
class StaticIntType:
    width: int
    signed: bool

    def __post_init__(self) -> None:
        if type(self.width) is not int or self.width <= 0:
            raise ValueError("ACPY-FAMILY-001: static integer width must be positive")
        if type(self.signed) is not bool:
            raise TypeError("ACPY-FAMILY-001: static integer signedness must be bool")


@dataclass(frozen=True, slots=True)
class StaticEnumType:
    enum_type: type[Enum]

    def __post_init__(self) -> None:
        if not isinstance(self.enum_type, type) or not issubclass(
            self.enum_type, Enum
        ):
            raise TypeError("ACPY-FAMILY-001: static_enum requires an Enum type")
        if not tuple(self.enum_type):
            raise ValueError("ACPY-FAMILY-001: static enum must be non-empty")


@dataclass(frozen=True, slots=True)
class StaticConfigType:
    config_type: type[object]

    def __post_init__(self) -> None:
        if not isinstance(self.config_type, type) or not getattr(
            self.config_type, "compiler_config__", False
        ):
            raise TypeError("ACPY-FAMILY-001: static_config requires an @ac.config type")
        if not dataclasses.is_dataclass(self.config_type):
            raise TypeError("ACPY-FAMILY-001: static config must be immutable")
        parameters = getattr(self.config_type, "__dataclass_params__", None)
        if parameters is None or not parameters.frozen:
            raise TypeError("ACPY-FAMILY-001: static config must be immutable")


StaticParameterType = StaticBoolType | StaticIntType | StaticEnumType | StaticConfigType


@dataclass(frozen=True, slots=True)
class OneOfConstraint:
    values: tuple[object, ...]

    def __post_init__(self) -> None:
        if not self.values:
            raise ValueError("ACPY-FAMILY-002: one_of requires at least one value")
        for index, value in enumerate(self.values):
            if value in self.values[:index]:
                raise ValueError("ACPY-FAMILY-002: one_of values must be unique")


@dataclass(frozen=True, slots=True)
class IntegerRangeConstraint:
    minimum: int
    maximum: int

    def __post_init__(self) -> None:
        if type(self.minimum) is not int or type(self.maximum) is not int:
            raise TypeError("ACPY-FAMILY-002: integer_range bounds must be integers")
        if self.minimum > self.maximum:
            raise ValueError("ACPY-FAMILY-002: integer_range bounds are inverted")


StaticConstraint = OneOfConstraint | IntegerRangeConstraint


def _fits_integer(value: int, kind: StaticIntType) -> bool:
    if kind.signed:
        minimum = -(1 << (kind.width - 1))
        maximum = (1 << (kind.width - 1)) - 1
    else:
        minimum = 0
        maximum = (1 << kind.width) - 1
    return minimum <= value <= maximum


def _check_value(kind: StaticParameterType, value: object) -> None:
    if isinstance(kind, StaticBoolType):
        if type(value) is not bool:
            raise TypeError("ACPY-FAMILY-003: static Boolean value has wrong type")
        return
    if isinstance(kind, StaticIntType):
        if type(value) is not int or not _fits_integer(value, kind):
            raise ValueError("ACPY-FAMILY-003: static integer value is out of range")
        return
    if isinstance(kind, StaticEnumType):
        if type(value) is not kind.enum_type:
            raise TypeError("ACPY-FAMILY-003: static enum value has wrong nominal type")
        return
    if type(value) is not kind.config_type:
        raise TypeError("ACPY-FAMILY-003: static config value has wrong nominal type")


@dataclass(frozen=True, slots=True)
class StaticParameter:
    name: str
    type: StaticParameterType
    default: object = _MISSING
    constraints: tuple[StaticConstraint, ...] = ()

    def __post_init__(self) -> None:
        if not isinstance(self.name, str) or not self.name.isidentifier():
            raise ValueError("ACPY-FAMILY-004: static parameter name must be an identifier")
        if not isinstance(
            self.type, (StaticBoolType, StaticIntType, StaticEnumType, StaticConfigType)
        ):
            raise TypeError("ACPY-FAMILY-004: unsupported static parameter type")
        if type(self.constraints) is not tuple or not all(
            isinstance(item, (OneOfConstraint, IntegerRangeConstraint))
            for item in self.constraints
        ):
            raise TypeError("ACPY-FAMILY-004: constraints must be a tuple")
        if self.default is not _MISSING:
            _check_value(self.type, self.default)
        for constraint in self.constraints:
            if isinstance(constraint, IntegerRangeConstraint):
                if not isinstance(self.type, StaticIntType):
                    raise TypeError(
                        "ACPY-FAMILY-004: integer_range requires a static integer"
                    )
                _check_value(self.type, constraint.minimum)
                _check_value(self.type, constraint.maximum)
            else:
                for value in constraint.values:
                    _check_value(self.type, value)
        if self.default is not _MISSING and not self.accepts(self.default):
            raise ValueError("ACPY-FAMILY-004: default violates its constraints")

    @property
    def required(self) -> bool:
        return self.default is _MISSING

    def accepts(self, value: object) -> bool:
        _check_value(self.type, value)
        for constraint in self.constraints:
            if isinstance(constraint, OneOfConstraint) and value not in constraint.values:
                return False
            if isinstance(constraint, IntegerRangeConstraint) and not (
                constraint.minimum <= value <= constraint.maximum
            ):
                return False
        return True


@dataclass(frozen=True, slots=True)
class StaticCase:
    bindings: tuple[tuple[str, object], ...]

    def __post_init__(self) -> None:
        if type(self.bindings) is not tuple:
            raise TypeError("ACPY-FAMILY-005: case bindings must be a tuple")
        names: set[str] = set()
        for binding in self.bindings:
            if (
                type(binding) is not tuple
                or len(binding) != 2
                or not isinstance(binding[0], str)
                or not binding[0].isidentifier()
            ):
                raise TypeError(
                    "ACPY-FAMILY-005: each case binding must be a (name, value) tuple"
                )
            if binding[0] in names:
                raise ValueError("ACPY-FAMILY-005: case binding names must be unique")
            names.add(binding[0])


def normalize_family_cases(
    parameters: tuple[StaticParameter, ...],
    finite_cases: tuple[StaticCase, ...] | None,
) -> tuple[StaticCase, ...]:
    """Validate and canonicalize the complete source-owned finite case set."""

    if type(parameters) is not tuple or not all(
        isinstance(parameter, StaticParameter) for parameter in parameters
    ):
        raise TypeError("ACPY-FAMILY-006: parameters must be a tuple literal")
    parameter_names = tuple(parameter.name for parameter in parameters)
    if len(set(parameter_names)) != len(parameter_names):
        raise ValueError("ACPY-FAMILY-006: static parameter names must be unique")
    if not parameters:
        if finite_cases is None:
            return (StaticCase(()),)
        if type(finite_cases) is not tuple or finite_cases != (StaticCase(()),):
            raise ValueError(
                "ACPY-FAMILY-006: zero-parameter family admits only case()"
            )
        return finite_cases
    if finite_cases is None or type(finite_cases) is not tuple or not finite_cases:
        raise ValueError(
            "ACPY-FAMILY-006: parameterized module requires non-empty finite_cases"
        )
    if not all(isinstance(item, StaticCase) for item in finite_cases):
        raise TypeError("ACPY-FAMILY-006: finite_cases must be a tuple of case() values")

    canonical: list[StaticCase] = []
    seen: set[tuple[object, ...]] = set()
    for source_case in finite_cases:
        supplied = dict(source_case.bindings)
        unknown = tuple(name for name in supplied if name not in parameter_names)
        if unknown:
            raise ValueError(
                f"ACPY-FAMILY-006: finite case has unknown binding {unknown[0]!r}"
            )
        values: list[object] = []
        bindings: list[tuple[str, object]] = []
        for parameter in parameters:
            if parameter.name in supplied:
                value = supplied[parameter.name]
            elif parameter.required:
                raise ValueError(
                    f"ACPY-FAMILY-006: finite case is missing required binding {parameter.name!r}"
                )
            else:
                value = parameter.default
            if not parameter.accepts(value):
                raise ValueError(
                    f"ACPY-FAMILY-006: finite case binding {parameter.name!r} violates constraints"
                )
            values.append(value)
            bindings.append((parameter.name, value))
        key = tuple(values)
        try:
            duplicate = key in seen
            seen.add(key)
        except TypeError as error:
            raise TypeError(
                "ACPY-FAMILY-006: static case values must be immutable and hashable"
            ) from error
        if duplicate:
            raise ValueError("ACPY-FAMILY-006: finite cases must be unique")
        canonical.append(StaticCase(tuple(bindings)))
    return tuple(canonical)


def static_bool() -> StaticBoolType:
    return StaticBoolType()


def static_int(*, width: int, signed: bool) -> StaticIntType:
    return StaticIntType(width, signed)


def static_enum(enum_type: type[Enum], /) -> StaticEnumType:
    return StaticEnumType(enum_type)


def static_config(config_type: type[object], /) -> StaticConfigType:
    return StaticConfigType(config_type)


def static_parameter(
    name: str,
    type: StaticParameterType,
    /,
    *,
    default: object = _MISSING,
    constraints: tuple[StaticConstraint, ...] = (),
) -> StaticParameter:
    return StaticParameter(name, type, default, constraints)


def one_of(*values: object) -> OneOfConstraint:
    return OneOfConstraint(values)


def integer_range(min: int, max: int, /) -> IntegerRangeConstraint:
    return IntegerRangeConstraint(min, max)


def case(*bindings: tuple[str, object]) -> StaticCase:
    return StaticCase(bindings)
