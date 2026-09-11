"""Shared cell-access contracts for one PE's block-group fabric.

``CellReadReq`` and ``CellWriteReq`` are the records that cross the whole BGF
seam, so they are shared rather than owned by one child: MAP writes the
placement fields, RQ and WQ hold the records without reading them, and ARB
consumes ``bank`` to select a conflict tree.  The placement geometry lives
here for the same reason — one decode profile, one set of constants.

Clients address cells, not banks.  CUBE, VEC and TLSU supply ``cell_key``
and identity; ``source_class``, ``bank``, ``row`` and
``out_of_range`` are written by designs/davincioo/tmu/bgf/map.py and must not
be supplied by a source.  Bank geometry therefore stays private to BGF and a
geometry change touches no client.

Bank geometry is also private *within* BGF: MAP decodes it and ARB consumes
it, and nothing between them is elaborated per bank.  ``bank`` rides in the
payload across RQ and WQ precisely so those queues need no bank dimension.

Access granularity is one whole 128-byte cell.  Clients see nothing smaller, so
these records carry no ``element_begin``, ``element_count`` or ``byte_mask``:
those describe sub-cell extents that cannot be requested, and they belong to the
upper memory-access path where they still appear (designs/davincioo/mem/noc/
xbar.py).  A sub-cell field here would invite a consumer to honour a request
shape the clients never issue.

``payload`` is still ``ac.u64`` and therefore holds one sixteenth of a cell,
which no longer matches the storage below it.  BANK now stores one whole cell per
entry and carries it as sixteen ``u64`` fields, so these BGF records are the
remaining narrow point on the path.  Widening them is mechanical -- sixteen
scalar fields, exactly as designs/davincioo/contracts/tmu_trf.py declares them --
and is tracked as an open item in designs/davincioo/tmu/bgf/arb.md rather than
done here, because it touches MAP, RQ, WQ, ARB and their tests together.
"""

from __future__ import annotations

import agentic_circuit as ac

# Banks private to one PE.  designs/davincioo/tmu/trf/bank.md records 32
# single-port 128-byte banks in private groups across the four PEs.
BANK_PARTITIONS = 8

# Provisional placement decode: a low-order bank interleave over ``cell_key``.
# designs/davincioo/tmu/bgf/map.md has not frozen ``mapping_mode``, so only this
# one mode is elaborated.  BANK_PARTITIONS must stay a power of two: apply and
# route expressions provide ``&`` and ``>>`` but neither ``%`` nor ``//``.
BANK_INDEX_BITS = 3
BANK_INDEX_MASK = BANK_PARTITIONS - 1


# Residency depth of one client class in RQ or WQ.  RQ and WQ hold one queue
# per class, not one per (class, bank), so this depth is the whole in-flight
# allowance of a class.  BANK_PARTITIONS is the useful bound: ARB can retire at
# most one request per bank per cycle, so a class with more than
# BANK_PARTITIONS resident requests cannot have them all conflict-free.  It is
# a starting point, not a frozen sizing; bgf/rq.md still owns that decision.
CLASS_RESIDENCY_DEPTH = BANK_PARTITIONS

# Closed source-class tags; they are written by MAP, ride through RQ and WQ
# untouched, and reach BankGrantReq.
SOURCE_CLASS_CUBE = 0
SOURCE_CLASS_VEC = 1
SOURCE_CLASS_TLSU = 2
SOURCE_CLASS_COUNT = 3

# MAP tags a lane by indexing its collection, and ``ac.map`` keys must be
# literals, so the tag values are also the lane ordinals.  This check keeps the
# literal keys in map.py and these names from drifting apart.
assert (SOURCE_CLASS_CUBE, SOURCE_CLASS_VEC, SOURCE_CLASS_TLSU) == (0, 1, 2)


@ac.struct
class CellReadReq:
    sequence: ac.u16
    source_class: ac.u8
    requester: ac.u8
    flow_id: ac.u16
    launch_generation: ac.u16
    thread_id: ac.u16
    block_id: ac.u16
    operation_id: ac.u16
    tile_id: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    request_id: ac.u16
    cell_key: ac.u16
    bank: ac.u16
    row: ac.u16
    out_of_range: bool
    response_route: ac.u8
    ordering_tag: ac.u16
    definedness_tag: ac.u16
    cancelled: bool


@ac.struct
class CellWriteReq:
    sequence: ac.u16
    source_class: ac.u8
    requester: ac.u8
    flow_id: ac.u16
    launch_generation: ac.u16
    thread_id: ac.u16
    block_id: ac.u16
    operation_id: ac.u16
    tile_id: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    request_id: ac.u16
    cell_key: ac.u16
    bank: ac.u16
    row: ac.u16
    out_of_range: bool
    response_route: ac.u8
    payload: ac.u64
    ordering_tag: ac.u16
    definedness_tag: ac.u16
    all_or_none: bool
    cancelled: bool
