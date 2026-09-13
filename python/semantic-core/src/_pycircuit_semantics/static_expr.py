"""Immutable elaboration-time integer expressions for dependent value types."""

from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import TypeAlias, Union


class StaticIntExpressionError(ValueError):
    """A static integer expression is malformed or cannot be resolved."""


StaticIntOperand: TypeAlias = Union[int, "StaticIntExpression"]

_INT64_MIN = -(1 << 63)
_INT64_MAX = (1 << 63) - 1


def _checked_i64(value: int, context: str) -> int:
    if not _INT64_MIN <= value <= _INT64_MAX:
        raise StaticIntExpressionError(
            f"static integer {context} exceeds the signed i64 domain"
        )
    return value


def _operand(value: object) -> StaticIntOperand:
    if type(value) is int:
        return value
    if isinstance(value, StaticIntExpression):
        return value
    raise TypeError("static integer expression operands must be integers or parameters")


@dataclass(frozen=True, slots=True)
class StaticIntExpression:
    """A closed symbolic integer expression resolved during specialization."""

    operation: str
    operands: tuple[StaticIntOperand, ...]
    name: str | None = None

    def __post_init__(self) -> None:
        if self.operation == "parameter":
            if not isinstance(self.name, str) or not self.name or self.operands:
                raise StaticIntExpressionError(
                    "static integer parameter requires one non-empty name"
                )
            return
        if self.name is not None:
            raise StaticIntExpressionError(
                "only static integer parameters carry a name"
            )
        arity = {"add": 2, "sub": 2, "mul": 2, "index_width": 1, "count_width": 1}
        if self.operation not in arity or len(self.operands) != arity[self.operation]:
            raise StaticIntExpressionError(
                f"invalid static integer expression operation {self.operation!r}"
            )
        for operand in self.operands:
            _operand(operand)

    @classmethod
    def parameter(cls, name: str) -> StaticIntExpression:
        return cls("parameter", (), name)

    def __add__(self, other: object) -> StaticIntExpression:
        return StaticIntExpression("add", (self, _operand(other)))

    def __radd__(self, other: object) -> StaticIntExpression:
        return StaticIntExpression("add", (_operand(other), self))

    def __sub__(self, other: object) -> StaticIntExpression:
        return StaticIntExpression("sub", (self, _operand(other)))

    def __rsub__(self, other: object) -> StaticIntExpression:
        return StaticIntExpression("sub", (_operand(other), self))

    def __mul__(self, other: object) -> StaticIntExpression:
        return StaticIntExpression("mul", (self, _operand(other)))

    def __rmul__(self, other: object) -> StaticIntExpression:
        return StaticIntExpression("mul", (_operand(other), self))

    def canonical(self) -> dict[str, object]:
        if self.operation == "parameter":
            return {"kind": "parameter", "name": self.name, "version": 1}
        return {
            "kind": "expression",
            "operation": self.operation,
            "operands": [
                operand if type(operand) is int else operand.canonical()
                for operand in self.operands
            ],
            "version": 1,
        }

    def postfix(self) -> tuple[str, ...]:
        """Return the canonical verifier program for this expression."""

        if self.operation == "parameter":
            assert self.name is not None
            return (f"param:{self.name}",)
        tokens: list[str] = []
        for operand in self.operands:
            if type(operand) is int:
                tokens.append(f"literal:{_checked_i64(operand, 'literal')}")
            else:
                tokens.extend(operand.postfix())
        tokens.append(self.operation)
        return tuple(tokens)

    def evaluate(self, bindings: Mapping[str, int]) -> int:
        if self.operation == "parameter":
            assert self.name is not None
            try:
                value = bindings[self.name]
            except KeyError as error:
                raise StaticIntExpressionError(
                    f"unbound static integer parameter {self.name!r}"
                ) from error
            if type(value) is not int:
                raise StaticIntExpressionError(
                    f"static integer parameter {self.name!r} must bind an integer"
                )
            return _checked_i64(value, f"parameter {self.name!r}")

        values = tuple(
            _checked_i64(operand, "literal")
            if type(operand) is int
            else operand.evaluate(bindings)
            for operand in self.operands
        )
        if self.operation == "add":
            return _checked_i64(values[0] + values[1], "addition")
        if self.operation == "sub":
            return _checked_i64(values[0] - values[1], "subtraction")
        if self.operation == "mul":
            return _checked_i64(values[0] * values[1], "multiplication")
        if values[0] <= 0:
            raise StaticIntExpressionError(
                f"{self.operation} requires a positive capacity"
            )
        if self.operation == "index_width":
            return max(1, (values[0] - 1).bit_length())
        if self.operation == "count_width":
            return max(1, values[0].bit_length())
        raise AssertionError("unreachable static integer expression")


def index_width(value: StaticIntOperand) -> int | StaticIntExpression:
    value = _operand(value)
    if type(value) is int:
        if value <= 0:
            raise StaticIntExpressionError("index_width requires a positive capacity")
        return max(1, (value - 1).bit_length())
    return StaticIntExpression("index_width", (value,))


def count_width(value: StaticIntOperand) -> int | StaticIntExpression:
    value = _operand(value)
    if type(value) is int:
        if value <= 0:
            raise StaticIntExpressionError("count_width requires a positive capacity")
        return max(1, value.bit_length())
    return StaticIntExpression("count_width", (value,))
