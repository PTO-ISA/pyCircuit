# ndf: node=ndf://davincioo/DAV-TMU-BGF-RQ-0001
"""Parent-composed read-queue schema for one PE's block-group fabric.

RQ deliberately contains no policy or storage owner of its own.  The BGF
assembly owns the queue state; this file describes the typed transport seam.

One elaboration of ``bgf_rq_system`` covers exactly one PE.  The four PE
control flows in designs/davincioo/ARCHITECTURE.md are physically independent
and own private bank groups, so PE identity is expressed by instantiating this
system once per PE rather than by a routing dimension inside it.  Arbitration
is likewise per PE: nothing in this seam is shared across PEs.

Within one PE the partition is ``source_class`` alone.  RQ holds one queue per
client class and has no bank dimension: bank is a scheduling coordinate, and
scheduling belongs to designs/davincioo/tmu/bgf/arb.py.  ``bank`` and ``row``
travel in the payload — decoded once by designs/davincioo/tmu/bgf/map.py and
read once by ARB — so RQ transports them without inspecting them.  The bank
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
    CellReadReq,
)


@ac.system
def bgf_rq_system(
    cube_read: CellReadReq,
    vec_read: CellReadReq,
    tlsu_read: CellReadReq,
) -> tuple[
    CellReadReq,
    CellReadReq,
    CellReadReq,
]:
    """Hold one independent read stream per source class.

    Each class gets ``CLASS_RESIDENCY_DEPTH`` slots of its own, so a class that
    stalls against ARB cannot consume another class's residency.  That is the
    only isolation RQ provides, and it is the only isolation RQ can provide:
    per-bank independence is a property of the crossbar in arb.py, which buffers
    every ``(class, bank)`` edge separately.

    The record passes through unchanged.  The transform carries the depth — the
    queue front end has no standalone buffer operator and an external port is
    always ``depth 1`` — and its identity body is what makes "RQ decides
    nothing" checkable in the lowered IR rather than only stated here.
    """

    cube_resident = cube_read.apply(
        lambda request: request,
        depth=CLASS_RESIDENCY_DEPTH,
        latency=1,
    )
    vec_resident = vec_read.apply(
        lambda request: request,
        depth=CLASS_RESIDENCY_DEPTH,
        latency=1,
    )
    tlsu_resident = tlsu_read.apply(
        lambda request: request,
        depth=CLASS_RESIDENCY_DEPTH,
        latency=1,
    )
    return (
        cube_resident,
        vec_resident,
        tlsu_resident,
    )
