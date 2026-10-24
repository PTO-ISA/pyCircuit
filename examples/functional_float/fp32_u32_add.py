"""Generated fixed-width integer add used by the TLSU/TADD consumer.

This is intentionally a tiny public source-unit model: it carries the U32
wraparound semantics into the pyCircuit compile/link/emit path instead of
reimplementing release arithmetic in the host consumer.
"""

import pycircuit as ac


@ac.module
def U32Add(lhs: ac.u32, rhs: ac.u32) -> ac.u32:  # noqa: N802
    return lhs + rhs
