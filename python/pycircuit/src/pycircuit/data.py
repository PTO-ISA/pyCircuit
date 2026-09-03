from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Generic, TypeVar

DT = TypeVar("DT", bound="Data", covariant=True)


@dataclass(frozen=True, eq=False)
class Data(ABC):
    """Structured signal type carried by ``Signal.ty``.

    ``str(Data)`` yields the canonical MLIR type literal so that f-string
    interpolation (``f"{sig.ty}"``) emits the same text as before.

    All concrete subclasses expose ``.width`` as the integer bit-width
    (vectors: leaf/element width; ``Clock``/``Reset``: 1).
    """

    def __eq__(self, other: object) -> bool:
        if isinstance(other, str):
            return str(self) == other
        if isinstance(other, Data):
            return type(self) is type(other) and self.__dict__ == other.__dict__
        return NotImplemented

    def __hash__(self) -> int:
        return hash(str(self))

    @property
    @abstractmethod
    def width(self) -> int:
        """Integer bit-width (vectors: leaf/element width; clock/reset: 1)."""

    @classmethod
    def from_str(cls, s: str) -> Data:
        raw = str(s).strip()
        if raw.startswith("vector<") and raw.endswith(">"):
            return Vector.from_str(raw)
        if raw.startswith("i"):
            return Bits.from_str(raw)
        if raw == "!pyc.clock":
            return Clock()
        if raw == "!pyc.reset":
            return Reset()
        raise ValueError(f"unsupported type literal: {s!r}")

    @abstractmethod
    def __str__(self) -> str:  # pragma: no cover - overridden by subclasses
        raise NotImplementedError


@dataclass(frozen=True, eq=False)
class Bits(Data):
    bitwidth: int

    def __post_init__(self) -> None:
        if not isinstance(self.bitwidth, int) or self.bitwidth <= 0:
            raise ValueError(
                f"Bits.bitwidth must be a positive int, got {self.bitwidth!r}"
            )

    @property
    def width(self) -> int:
        return self.bitwidth

    def __str__(self) -> str:
        return f"i{self.bitwidth}"

    @classmethod
    def from_str(cls, s: str) -> Bits:
        raw = str(s).strip()
        if not raw.startswith("i"):
            raise ValueError(f"invalid bits type: {s!r}")
        tail = raw[1:]
        if not (tail and tail.isdigit()):
            raise ValueError(f"invalid bits type: {s!r}")
        w = int(tail)
        if w <= 0:
            raise ValueError(f"invalid bits type: {s!r}")
        return cls(w)


@dataclass(frozen=True, eq=False)
class Vector(Data, Generic[DT]):
    length: int
    elem: DT

    def __post_init__(self) -> None:
        if not isinstance(self.length, int) or self.length <= 0:
            raise ValueError(
                f"Vector.length must be a positive int, got {self.length!r}"
            )
        if not isinstance(self.elem, Data):
            raise TypeError(
                f"Vector.elem must be a Data, got {type(self.elem).__name__}"
            )

    def __str__(self) -> str:
        shape: list[int] = [self.length]
        e: Data = self.elem
        while isinstance(e, Vector):
            shape.append(e.length)
            e = e.elem
        return "vector<" + "x".join(str(d) for d in shape) + "x" + str(e) + ">"

    @property
    def width(self) -> int:
        """Integer width of the element (leaf) type."""
        return self.datatype().width

    def shape(self) -> list[int]:
        """All dimensions flattened, outer-to-inner. ``Vector(4, Vector(3, Bits(8)))`` → ``[4, 3]``."""
        s: list[int] = [self.length]
        e: Data = self.elem
        while isinstance(e, Vector):
            s.append(e.length)
            e = e.elem
        return s

    def datatype(self) -> Data:
        """Innermost non-Vector element. ``Vector(4, Vector(3, Bits(8)))`` → ``Bits(8)``."""
        e: Data = self.elem
        while isinstance(e, Vector):
            e = e.elem
        return e

    @classmethod
    def from_str(cls, s: str) -> Vector[Data]:
        raw = str(s).strip()
        if not (raw.startswith("vector<") and raw.endswith(">")):
            raise ValueError(f"expected vector type, got {raw!r}")
        body = raw[len("vector<") : -1]
        parts: list[str] = []
        depth = 0
        start = 0
        for i, ch in enumerate(body):
            if ch == "<":
                depth += 1
            elif ch == ">":
                depth -= 1
            elif ch == "x" and depth == 0:
                parts.append(body[start:i].strip())
                start = i + 1
        parts.append(body[start:].strip())
        if len(parts) < 2:
            raise ValueError(f"invalid vector type body: {body!r}")
        dims: list[int] = []
        for p in parts[:-1]:
            lanes = int(p)
            if lanes <= 0:
                raise ValueError(f"vector lanes must be > 0, got {lanes}")
            dims.append(lanes)
        return cls.from_shape(dims, Data.from_str(parts[-1]))

    @classmethod
    def from_shape(cls, shape: list[int], leaf: Data) -> Vector[Data]:
        """Wrap ``leaf`` in nested ``Vector`` from outer to inner.

        ``cls.from_shape([4, 3], Bits(8))`` → ``Vector(4, Vector(3, Bits(8)))``.
        """
        if not shape:
            raise ValueError("shape must be non-empty")
        node: Data = leaf
        for d in reversed(shape):
            node = Vector(d, node)
        if not isinstance(node, Vector):
            raise ValueError("shape must be non-empty")
        return node


@dataclass(frozen=True, eq=False)
class Clock(Data):
    def __str__(self) -> str:
        return "!pyc.clock"

    @property
    def width(self) -> int:
        return 1


@dataclass(frozen=True, eq=False)
class Reset(Data):
    def __str__(self) -> str:
        return "!pyc.reset"

    @property
    def width(self) -> int:
        return 1
