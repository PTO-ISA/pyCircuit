"""A JIT-bound exact-width scalar crosses the typed system boundary."""

from __future__ import annotations

import agentic_circuit as ac


WIDTH = ac.param[int]("width")


@ac.rule
def keep(value: ac.bits[WIDTH]) -> ac.bits[WIDTH]:
    return value


@ac.system
def scalar_parameterized_types(
    value: ac.bits[WIDTH], *, width: ac.const[int]
) -> ac.bits[WIDTH]:
    result = keep(value)
    return result


specialization = ac.jit(scalar_parameterized_types, width=17)
