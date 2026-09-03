"""Type-safe enumerations for PyCircuit (TODO T3, ASL ``enumeration {...}``).

ASL enums are named types that are **not** interchangeable with bare integers;
they guard against typos, insufficient width, and cross-enum comparison. This
module brings the same discipline to PyCircuit signals as pure elaboration-time
sugar (no MLIR dialect change):

- :class:`PycEnum` — a base class whose members get 0-based codes (via
  ``auto()``) and from which a minimal bit ``width`` is derived.
- :class:`EnumSignal` — a thin tag attaching an enum *type* to a signal. Its
  :meth:`~EnumSignal.is_` expands to ``sig == const(code)`` and rejects
  comparisons against bare ints or members of a *different* enum.

Example::

    from pycircuit import PycEnum, auto

    class SRType(PycEnum):
        LSL = auto(); LSR = auto(); ASR = auto(); ROR = auto()

    SRType.width                 # 2  == ceil(log2(4))
    op  = m.input("op", enum=SRType)     # width auto = SRType.width
    hit = op.is_(SRType.LSL)             # i1; op.is_(0) / op == 0 -> TypeError
"""

from __future__ import annotations

import enum
from enum import EnumMeta, auto
from typing import Any

__all__ = ["PycEnum", "EnumSignal", "auto", "enum_width", "enumeration"]


def enum_width(cls: type) -> int:
    """Minimum unsigned bit width needed to encode every member of ``cls``.

    ``max(1, floor(log2(max_code)) + 1)``; a single-member enum still needs 1
    bit. Codes must be non-negative plain ints.
    """
    if not (isinstance(cls, _PycEnumMeta) and len(cls.__members__) > 0):
        raise TypeError("enum_width() expects a non-empty PycEnum subclass")
    max_code = 0
    for member in cls:
        code = member.value
        if isinstance(code, bool) or not isinstance(code, int):
            raise TypeError(
                f"{cls.__name__}.{member.name} has non-int code {code!r}; "
                "PycEnum codes must be plain ints"
            )
        if code < 0:
            raise ValueError(
                f"{cls.__name__}.{member.name} has negative code {code}; "
                "PycEnum codes must be >= 0"
            )
        if code > max_code:
            max_code = code
    return max(1, max_code.bit_length())


class _PycEnumMeta(EnumMeta):
    """Metaclass adding a class-level ``width`` (``SRType.width``)."""

    @property
    def width(cls) -> int:
        return enum_width(cls)


class PycEnum(enum.Enum, metaclass=_PycEnumMeta):
    """Base class for type-safe, 0-based hardware enumerations.

    Subclass it and give each member ``auto()`` (0-based) or an explicit
    non-negative int code::

        class SRType(PycEnum):
            LSL = auto(); LSR = auto(); ASR = auto(); ROR = auto()

    ``SRType.width`` is the minimal encoding width; ``SRType.LSL.const(ctx)``
    materializes a width-correct constant signal; ``SRType.bind(sig)`` tags an
    existing signal so it gains :meth:`EnumSignal.is_`.
    """

    def _generate_next_value_(name, start, count, last_values):  # type: ignore[override]  # noqa: N805
        # 0-based codes for tight encoding (auto() -> 0, 1, 2, ...).
        return count

    @property
    def width(self) -> int:
        return enum_width(type(self))

    def const(self, ctx: Any) -> Any:
        """A width-correct constant signal for this member.

        ``ctx`` may be a ``Circuit`` (returns a ``Wire``) or a
        ``CycleAwareDomain`` (returns a ``CycleAwareSignal``).
        """
        w = enum_width(type(self))
        code = int(self.value)
        from .v6 import CycleAwareDomain, cas  # lazy: avoid import cycle

        if isinstance(ctx, CycleAwareDomain):
            return cas(ctx, ctx.create_const(code, width=w))
        const_fn = getattr(ctx, "const", None)
        if callable(const_fn):
            return const_fn(code, width=w)
        raise TypeError(
            "member.const(ctx) expects a Circuit or CycleAwareDomain, got "
            f"{type(ctx).__name__}"
        )

    @classmethod
    def bind(cls, signal: Any) -> EnumSignal:
        """Tag an existing signal with this enum type (returns an EnumSignal)."""
        return EnumSignal(cls, signal)


def enumeration(name: str, *members: Any) -> type:
    """ASL-style ``enumeration {RED, GREEN, BLUE}`` — a one-liner constructor.

    Build a :class:`PycEnum` subclass from **member names only** (0-based codes,
    auto width), mirroring ASL's ``type Color of enumeration {RED, GREEN, BLUE}``.
    Names may be given as varargs, one iterable, or a single comma/space
    -separated string -- all equivalent::

        Color = enumeration("Color", "RED", "GREEN", "BLUE")
        Color = enumeration("Color", ["RED", "GREEN", "BLUE"])
        Color = enumeration("Color", "RED GREEN BLUE")
        Color = enumeration("Color", "RED, GREEN, BLUE")

    Equivalent to the class form::

        class Color(PycEnum):
            RED = auto(); GREEN = auto(); BLUE = auto()

    Use the class form when you need explicit codes or docstrings.
    """
    if not isinstance(name, str) or not name.strip():
        raise TypeError("enumeration(name, ...): name must be a non-empty str")
    if len(members) == 1 and not isinstance(members[0], str):
        names = list(members[0])
    elif len(members) == 1:
        names = members[0].replace(",", " ").split()
    else:
        names = list(members)
    if not names:
        raise ValueError("enumeration requires at least one member name")
    for n in names:
        if not isinstance(n, str) or not n.isidentifier():
            raise ValueError(
                f"enumeration member name must be an identifier, got {n!r}"
            )
    if len(set(names)) != len(names):
        raise ValueError(f"enumeration {name!r} has duplicate member names: {names}")
    # Functional Enum API on the PycEnum base -> a 0-based PycEnum subclass that
    # inherits _generate_next_value_ and the _PycEnumMeta metaclass.
    return PycEnum(name, names)  # type: ignore[call-overload]


def coerce_enum_cls(enum_cls: object) -> type:
    """Validate ``enum=`` and return the enum class."""
    if (
        isinstance(enum_cls, _PycEnumMeta)
        and len(getattr(enum_cls, "__members__", {})) > 0
    ):
        return enum_cls  # type: ignore[return-value]
    raise TypeError("enum= must be a non-empty PycEnum subclass, got " f"{enum_cls!r}")


class EnumSignal:
    """A signal tagged with a :class:`PycEnum` type.

    Created by ``E.bind(signal)`` or ``m.input(..., enum=E)`` /
    ``domain.signal(..., enum=E)``. It deliberately exposes a *narrow* surface
    (unlike ``BitfieldSignal``): the sanctioned operations are :meth:`is_` /
    :meth:`is_not` (and ``==`` / ``!=`` as aliases), which only accept members
    of the *same* enum. Comparing to a bare int or a different enum raises
    ``TypeError`` at elaboration time. Use :attr:`raw` for bit-level ops.
    """

    __slots__ = ("_enum", "_signal")

    def __init__(self, enum_cls: type, signal: object) -> None:
        object.__setattr__(self, "_enum", coerce_enum_cls(enum_cls))
        object.__setattr__(self, "_signal", signal)

    @property
    def raw(self) -> object:
        """The underlying (untagged) signal."""
        return self._signal

    @property
    def enum(self) -> type:
        return self._enum

    @property
    def width(self) -> int:
        return enum_width(self._enum)

    def __pyc_unwrap__(self) -> object:
        """Hook used by ``wire_of`` / ``m.output`` / ``_to_wire``."""
        return self._signal

    def _member_code(self, member: object, *, op: str) -> int:
        if isinstance(member, PycEnum):
            if type(member) is self._enum:
                return int(member.value)
            raise TypeError(
                f"{op}: cannot compare {self._enum.__name__} signal with "
                f"{type(member).__name__}.{member.name} (different enum type)"
            )
        raise TypeError(
            f"{op}: {self._enum.__name__} signal is not comparable to "
            f"{type(member).__name__} {member!r}; use {self._enum.__name__}.<MEMBER>"
        )

    def is_(self, member: object) -> object:
        """``True`` iff the signal equals ``member`` (returns i1)."""
        return self._signal == self._member_code(member, op="is_")

    def is_not(self, member: object) -> object:
        """``True`` iff the signal differs from ``member`` (returns i1)."""
        return self._signal != self._member_code(member, op="is_not")

    def __eq__(self, other: object) -> object:  # type: ignore[override]
        return self.is_(other)

    def __ne__(self, other: object) -> object:  # type: ignore[override]
        return self.is_not(other)

    def _coerce_assign(self, other: object) -> object:
        if isinstance(other, PycEnum):
            if type(other) is not self._enum:
                raise TypeError(
                    f"cannot assign {type(other).__name__} member to a "
                    f"{self._enum.__name__} register"
                )
            return int(other.value)
        if isinstance(other, EnumSignal):
            if other._enum is not self._enum:
                raise TypeError(
                    f"cannot assign {other._enum.__name__} signal to a "
                    f"{self._enum.__name__} register"
                )
            return other._signal
        return other

    def __ilshift__(self, other: object) -> EnumSignal:
        self._signal.__ilshift__(self._coerce_assign(other))  # type: ignore[attr-defined]
        return self

    def assign(self, next_val: object, *, when: object = None) -> None:
        self._signal.assign(self._coerce_assign(next_val), when=when)  # type: ignore[attr-defined]

    def __hash__(self) -> int:
        return id(self)

    def __setattr__(self, name: str, value: object) -> None:
        raise AttributeError("EnumSignal is immutable")

    def __repr__(self) -> str:
        return f"EnumSignal({self._enum.__name__}, {self._signal!r})"
