"""Lowest-level semantic type rendering for Queue ACIR text."""

from __future__ import annotations

from _pycircuit_semantics import ValueType


def _render_type(value_type: ValueType) -> str:
    """Render one semantic value type only at the ACIR text boundary."""

    return value_type.mlir()
