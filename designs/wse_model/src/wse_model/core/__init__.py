"""The AICORE: pipelines, local DRAM, on-chip buffers, and Cube/Vector throughput.

Whitepaper §3 describes one AICORE as one Cube plus one Vector at 1.4 GHz, keeping
Davinci's pipeline organisation (scalar issue, MTE for movement, Cube/Vector for
arithmetic, FixPipe for write-back) and changing only the storage side. This
package models what the design actually specifies:

* :mod:`wse_model.core.pipelines` — the pipe set and the independent-dispatch
  requirement that makes Calendar overlappable (whitepaper §15.2, Calendar §4.7);
* :mod:`wse_model.core.dram` — the per-core local DRAM with its 2 KB page
  granularity, which is the real read/write unit (whitepaper §3.4);
* :mod:`wse_model.core.buffers` — the L0A/L0B/L0C/L1/UB hierarchy and its fit
  checks;
* :mod:`wse_model.core.cube` — the Cube and Vector throughput model that turns a
  tile shape into cycles, weight bytes, and a roofline verdict.

What is **not** modelled is stated rather than implied: weight layout and the
prefetch path into L0B are open item ``Q8``, so the local DRAM model exposes a
layout declaration instead of assuming one.
"""

from wse_model.core.buffers import (
    BUFFER_CAPACITY_BYTES,
    BufferKind,
    BufferResidency,
    TileRequirement,
    fit_check,
)
from wse_model.core.cube import (
    MatmulShape,
    MatmulTiming,
    VectorTiming,
    matmul_timing,
    vector_timing,
)
from wse_model.core.dram import LocalDram, LocalDramLayout
from wse_model.core.pipelines import (
    INDEPENDENT_FROM_MTE4,
    PIPE_ORDER_NOTE,
    Pipe,
    PipeBarrier,
    PipeTracker,
    StepRequirement,
    calendar_step_pipes,
)

__all__ = [
    "BUFFER_CAPACITY_BYTES",
    "INDEPENDENT_FROM_MTE4",
    "PIPE_ORDER_NOTE",
    "BufferKind",
    "BufferResidency",
    "LocalDram",
    "LocalDramLayout",
    "MatmulShape",
    "MatmulTiming",
    "Pipe",
    "PipeBarrier",
    "PipeTracker",
    "StepRequirement",
    "TileRequirement",
    "VectorTiming",
    "calendar_step_pipes",
    "fit_check",
    "matmul_timing",
    "vector_timing",
]
