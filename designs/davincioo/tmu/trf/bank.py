# ndf: node=ndf://davincioo/DAV-TMU-TRF-BANK-0001
"""Raw payload storage for one physical bank of one PE.

BANK is the sole owner of raw payload bytes.  It holds no descriptor, no
definedness state and no publication state: an acknowledgement here means the
bytes changed, never that a Tile became visible.  designs/davincioo/tmu/trn/
sts.md and rat.md own that meaning.

One elaboration of ``trf_bank_system`` covers one bank.  designs/davincioo/tmu/
trf/bank.md records 32 single-port 128-byte banks in private groups, so a PE
instantiates this system eight times and no state is shared between banks.

Two arrays are elaborated, both indexed by ``row`` alone.  One entry of the cell
array is one whole 128-byte cell, so the access granularity clients see is the
granularity the storage has, and no address arithmetic can reach a fragment of a
cell.  The generation array holds one generation per cell.

One rule touches both arrays, which is what makes this bank single-port: a rule
admits one token per tick, so at most one access reaches the arrays per cycle.
Reads, writes and invalidations share that one rule because they share the same
state, and each array has exactly one writer.

The generation check qualifies the write in the same tick.  A stale write is
demoted to a read of the same row, so an old generation cannot alter a reused
cell while the request still produces a response.  This leaf never drops a
request: the requester is waiting for one either way.

Storage is a persistent indexed variable, which Decision 0151 classifies as
provisional state.  That buys the correct granularity at a real cost: PYC and
RTL must reject this construct at an explicit boundary, so gfsim is the only
backend that can execute this leaf until Decision 0151 is superseded.  bank.md
records that boundary.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_trf import (
    ACCESS_INVALIDATE,
    ACCESS_WRITE,
    CELL_OUTPUT_STAGES,
    ROW_INDEX_MASK,
    ROWS_PER_BANK,
    RULE_ACCESS_LATENCY,
    BankAccess,
    CellData,
)


@ac.rule
def serve_bank_access(cells, generations, access):
    """Apply one access to one bank's two arrays, atomically.

    The generation of the addressed row decides whether a write may commit.
    ``current`` reports that decision and ``applied`` reports whether bytes
    actually changed, so a rejected write is observable instead of silent.

    An invalidation installs a new generation and touches no payload bytes.
    Clearing the cell would be wasted work: a generation mismatch already makes
    the old bytes unreachable, and no reader can observe them afterwards.

    The returned record carries the cell as it was committed before this tick,
    because a proposed write becomes visible only at tick commit.  A write
    therefore returns the cell it replaced, which is what makes a demoted stale
    write distinguishable from one that took effect.

    Identity comes from ``access`` and never from storage.  ``CellData`` holds no
    identity precisely so that a read cannot return the previous writer's
    identity and misroute the response.
    """

    row = access.row & ROW_INDEX_MASK
    stored = generations[row]
    live = stored == access.tile_version

    if live and access.kind == ACCESS_WRITE:
        cells[row] = CellData(
            word0=access.word0,
            word1=access.word1,
            word2=access.word2,
            word3=access.word3,
            word4=access.word4,
            word5=access.word5,
            word6=access.word6,
            word7=access.word7,
            word8=access.word8,
            word9=access.word9,
            word10=access.word10,
            word11=access.word11,
            word12=access.word12,
            word13=access.word13,
            word14=access.word14,
            word15=access.word15,
        )

    if access.kind == ACCESS_INVALIDATE:
        generations[row] = access.installed_generation

    observed = cells[row]
    return access.with_fields(
        stored_generation=stored,
        current=live,
        applied=live & (access.kind == ACCESS_WRITE),
        word0=observed.word0,
        word1=observed.word1,
        word2=observed.word2,
        word3=observed.word3,
        word4=observed.word4,
        word5=observed.word5,
        word6=observed.word6,
        word7=observed.word7,
        word8=observed.word8,
        word9=observed.word9,
        word10=observed.word10,
        word11=observed.word11,
        word12=observed.word12,
        word13=observed.word13,
        word14=observed.word14,
        word15=observed.word15,
    )


@ac.system
def trf_bank_system(access: BankAccess) -> BankAccess:
    """Serve at most one access per cycle for one bank.

    The single input stream matches what ARB grants: one access per bank per
    cycle, reads and writes already ordered against each other.  All three access
    kinds stay on that one stream because they contend for the same arrays;
    splitting them would create two ways into one physical port.

    ``row`` is masked before it indexes either array, so every access lands
    inside this bank's own arrays and Decision 0151's ``table_index_out_of_range``
    cannot fire.  That bound is structural rather than a promise from upstream,
    which matters because the bounds result that
    designs/davincioo/tmu/bgf/map.py computes is reported and never enforced.

    Masking bounds the access; it does not refuse it.  A row past ROWS_PER_BANK
    wraps onto a different valid row of this bank, so an out-of-range access
    reads or writes some other cell's payload instead of failing.  Refusing it
    needs an owner that does not exist yet; bank.md records this as open.

    The result is delayed so the read latency visible here is exactly
    CELL_READ_LATENCY: the rule contributes RULE_ACCESS_LATENCY and this stage
    supplies the remainder.
    """

    cells: list[CellData] = [0] * ROWS_PER_BANK
    generations: list[ac.u16] = [0] * ROWS_PER_BANK

    served = serve_bank_access(cells, generations, access)
    completed = ac.pipeline(
        served,
        stages=CELL_OUTPUT_STAGES,
        depth=1,
    )
    return completed


assert RULE_ACCESS_LATENCY == 1
