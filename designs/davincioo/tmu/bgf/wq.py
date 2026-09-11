# ndf: node=ndf://davincioo/DAV-TMU-BGF-WQ-0001
"""Parent-composed write-queue schema for one PE's block-group fabric.

WQ preserves complete masked writes.  Queue ownership remains in the BGF
assembly; this file only describes the typed transport seam.

One elaboration of ``bgf_wq_system`` covers exactly one PE.  The four PE
control flows in designs/davincioo/ARCHITECTURE.md are physically independent
and own private bank groups, so PE identity is expressed by instantiating this
system once per PE rather than by a routing dimension inside it.  Arbitration
is likewise per PE: nothing in this seam is shared across PEs.

Within one PE the partition is ``source_class`` alone.  WQ holds one queue per
client class and has no bank dimension: bank is a scheduling coordinate, and
scheduling belongs to designs/davincioo/tmu/bgf/arb.py.  ``bank`` and ``row``
travel in the payload — decoded once by designs/davincioo/tmu/bgf/map.py and
read once by ARB — so WQ transports them without inspecting them.  The bank
crossbar therefore exists exactly once, inside the leaf that resolves bank
conflicts, instead of being split across a partitioning seam and a merging one.

FlowKey stays in the payload for identity and cancellation matching, but takes
no part in routing; if a single PE ever hosts more than one ``stid``, a further
partition is required and this assumption must be revisited.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_bgf import (
    CLASS_RESIDENCY_DEPTH,
    CellWriteReq,
)


@ac.system
def bgf_wq_system(
    cube_write: CellWriteReq,
    vec_write: CellWriteReq,
    tlsu_write: CellWriteReq,
) -> tuple[
    CellWriteReq,
    CellWriteReq,
    CellWriteReq,
]:
    """Hold one independent write stream per source class.

    Each class gets ``CLASS_RESIDENCY_DEPTH`` slots of its own, so a class that
    stalls against ARB cannot consume another class's residency.  That is the
    only isolation WQ provides, and it is the only isolation WQ can provide:
    per-bank independence is a property of the crossbar in arb.py, which buffers
    every ``(class, bank)`` edge separately.

    The payload travels as one record; no split, coalesce, or
    partial transfer is introduced.  The transform carries the depth — the queue
    front end has no standalone buffer operator and an external port is always
    ``depth 1`` — and its identity body is what makes "WQ decides nothing"
    checkable in the lowered IR rather than only stated here.
    """

    cube_resident = cube_write.apply(
        lambda request: request,
        depth=CLASS_RESIDENCY_DEPTH,
        latency=1,
    )
    vec_resident = vec_write.apply(
        lambda request: request,
        depth=CLASS_RESIDENCY_DEPTH,
        latency=1,
    )
    tlsu_resident = tlsu_write.apply(
        lambda request: request,
        depth=CLASS_RESIDENCY_DEPTH,
        latency=1,
    )
    return (
        cube_resident,
        vec_resident,
        tlsu_resident,
    )
