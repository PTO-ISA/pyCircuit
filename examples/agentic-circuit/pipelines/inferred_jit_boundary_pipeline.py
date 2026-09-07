"""Typed runtime boundaries with one structural JIT choice."""

from __future__ import annotations

import agentic_circuit as ac


@ac.config
class BoundaryConfig:
    increment: bool


@ac.module
def add_one(value: ac.u8) -> ac.u8:
    return value + 1


@ac.module
def subtract_one(value: ac.u8) -> ac.u8:
    return value - 1


@ac.system
def inferred_jit_boundary_pipeline(
    value: ac.u8, *, cfg: ac.const[BoundaryConfig]
) -> ac.u8:
    if cfg.increment:
        result = add_one(value)
    else:
        result = subtract_one(value)
    return result


specialization = ac.jit(
    inferred_jit_boundary_pipeline,
    cfg=BoundaryConfig(increment=True),
)
