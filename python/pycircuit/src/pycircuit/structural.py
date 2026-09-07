"""Explicit low-level helpers for raw structural Wire authoring."""

from __future__ import annotations

from .hw import Reg, Wire
from .literals import LiteralValue


def mux(
    cond: Wire,
    true_value: Wire | Reg | int | LiteralValue,
    false_value: Wire | Reg | int | LiteralValue,
) -> Wire:
    """Select between raw structural values with an i1 Wire condition."""
    if not isinstance(cond, Wire):
        raise TypeError(
            f"structural.mux() condition must be a Wire, got {type(cond).__name__}"
        )
    return cond._select_internal(true_value, false_value)


__all__ = ["mux"]
