"""Ordinary nested module calls preserve reusable typed-family hierarchy."""

from __future__ import annotations

import agentic_circuit as ac


@ac.module_decl(source="examples/agentic-circuit/pipelines/inferred_nested_module_pipeline.py")
def increment(value: ac.u8) -> ac.u8:
    ...

increment_decl = increment

@ac.module(declaration=increment_decl)
def increment(value: ac.u8) -> ac.u8:
    return value + 1


@ac.module_decl(source="examples/agentic-circuit/pipelines/inferred_nested_module_pipeline.py")
def wrapper(value: ac.u8) -> ac.u8:
    ...

wrapper_decl = wrapper

@ac.module(declaration=wrapper_decl)
def wrapper(value: ac.u8) -> ac.u8:
    return increment(value)


@ac.system
def inferred_nested_module_pipeline(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
    left_result = wrapper(left)
    right_result = wrapper(right)
    return left_result, right_result
