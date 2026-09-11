# ndf: node=ndf://davincioo/DAV-TMU-BGF-MAP-0001
"""Placement decoder for one PE's block-group fabric.

MAP is the BGF entry stage and the only owner of the cell-to-placement
decision.  It tags the source class of each lane and turns ``cell_key`` into
``bank``, ``row`` and a bounds result; RQ and WQ then own residency and
partition on what MAP decided.  Keeping the decode here is what lets clients
address cells without knowing bank geometry.

One elaboration of ``bgf_map_system`` covers exactly one PE, matching RQ, WQ
and ARB.  The four PE control flows in designs/davincioo/ARCHITECTURE.md are
physically independent and own private bank groups, so PE identity is instance
geometry rather than a routing dimension, and no mapping state is shared
across PEs.

The decoder is pure: it appends placement fields and never replaces source
identity, drops a request, or reorders a lane.  ``out_of_range`` is reported,
not acted on — the owner of the rejection policy is a separate decision
recorded in map.md.

This profile decodes one mode only, the low-order interleave
``bank = cell_key & BANK_INDEX_MASK`` and ``row = cell_key >> BANK_INDEX_BITS``.
The scope base/range tables, ``mapping_mode`` and the Local/Shared geometries
in map.md are not frozen and are therefore not elaborated here.  The row bound is
imported from the shared contract, where designs/davincioo/tmu/trf/bank.md freezes
it at 256 rows.

Two frontend constraints shape the record.  ``bank`` and ``row`` are ``u16``
because a field update requires exact width equality and no narrowing or cast
primitive exists, so both must match the width of a ``cell_key`` expression
even though they need 3 and 13 bits.  The lane collections are keyed by
integer literals because ``ac.map`` keys must be compile-time literals; the
shared contract asserts that those literals are the ``SOURCE_CLASS_*`` values.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_bgf import (
    BANK_INDEX_BITS,
    BANK_INDEX_MASK,
    SOURCE_CLASS_COUNT,
    CellReadReq,
    CellWriteReq,
)

# The row bound is physical geometry that TRF owns, not a BGF choice.  BANK masks
# every row index to this range, so a bound declared here would let MAP report a
# row as in-range that BANK then folds onto a different cell.  bank.md freezes it
# at 256; the low-order interleave admits ``cell_key >> BANK_INDEX_BITS`` up to
# 8192 rows, so the bound stays well inside that range and ``out_of_range`` stays
# reachable instead of always false.
from designs.davincioo.contracts.tmu_trf import ROWS_PER_BANK


@ac.system
def bgf_map_system(
    cube_read: CellReadReq,
    vec_read: CellReadReq,
    tlsu_read: CellReadReq,
    cube_write: CellWriteReq,
    vec_write: CellWriteReq,
    tlsu_write: CellWriteReq,
) -> tuple[
    CellReadReq,
    CellReadReq,
    CellReadReq,
    CellWriteReq,
    CellWriteReq,
    CellWriteReq,
]:
    """Tag and decode one PE's six client lanes.

    Each lane is one ``(source_class, path)`` pair and keeps its own queue, so
    a blocked read lane cannot stall a write lane or another class.  The decode
    itself is combinational; the single registered stage per lane is the
    optional pipeline that map.md allows, and it replaces — rather than adds
    to — the tagging stage RQ and WQ used to own.

    ``bank`` is written here so RQ and WQ never recompute it.  Because it is a
    mask over ``BANK_INDEX_MASK`` it cannot leave ``[0, BANK_PARTITIONS)``, and
    the consumers reapply that mask in their route keys so the bound stays
    structural instead of resting on this contract alone.
    """

    reads = ac.map({0: cube_read, 1: vec_read, 2: tlsu_read})
    writes = ac.map({0: cube_write, 1: vec_write, 2: tlsu_write})
    mapped_reads = ac.array(
        SOURCE_CLASS_COUNT,
        lambda source_class: reads[source_class].apply(
            lambda request: request.with_fields(
                source_class=source_class,
                bank=request.cell_key & BANK_INDEX_MASK,
                row=request.cell_key >> BANK_INDEX_BITS,
                out_of_range=(request.cell_key >> BANK_INDEX_BITS) >= ROWS_PER_BANK,
            ),
            depth=1,
            latency=1,
        ),
    )
    mapped_writes = ac.array(
        SOURCE_CLASS_COUNT,
        lambda source_class: writes[source_class].apply(
            lambda request: request.with_fields(
                source_class=source_class,
                bank=request.cell_key & BANK_INDEX_MASK,
                row=request.cell_key >> BANK_INDEX_BITS,
                out_of_range=(request.cell_key >> BANK_INDEX_BITS) >= ROWS_PER_BANK,
            ),
            depth=1,
            latency=1,
        ),
    )
    return (
        mapped_reads[0],
        mapped_reads[1],
        mapped_reads[2],
        mapped_writes[0],
        mapped_writes[1],
        mapped_writes[2],
    )
