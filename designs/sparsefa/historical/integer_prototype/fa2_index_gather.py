# ruff: noqa: N802, F821, F841 -- Module names are published hardware blocks;
# captured state assignments are compiler-owned.
"""Integer sparse-KV address path for the FlashAttention2 bring-up.

This source deliberately stops at token address generation.  It does not
implement floating point QK/PV arithmetic or an index-selection algorithm.
Indices are supplied by the caller, stored in a bounded per-query list, then
validated and translated through the logical-token to physical-K/V map.
"""

import pycircuit as ac


@ac.struct
class IndexWord:
    """One externally supplied sparse index list item."""

    position: ac.u4
    token: ac.u11


@ac.struct
class IndexStoreResult:
    token: ac.u11
    count: ac.u5
    read_valid: ac.u1
    write_accepted: ac.u1
    duplicate: ac.u1
    count_error: ac.u1


@ac.module
def IndexStore(
    write_valid: ac.u1,
    write: IndexWord,
    clear_valid: ac.u1,
    write_count_valid: ac.u1,
    write_count: ac.u5,
    read_valid: ac.u1,
    read_position: ac.u4,
) -> IndexStoreResult:
    """Own the captured index list and its committed active count.

    A producer writes positions in any order during a query setup window and
    commits the active prefix with ``write_count_valid``.  The table read is a
    synchronous design-facing lookup in the surrounding pipeline; this small
    source module exposes the committed token and lets the compiler derive the
    actual storage and transaction controls.
    """

    entries: ac.table[16, ac.u11] = (
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    )
    count: ac.u5 = 0
    duplicate: ac.u1 = 0
    count_error: ac.u1 = 0

    duplicate_hit = (~clear_valid) & (
        (write.position != 0) & (write.token == entries[0])
        | (write.position != 1) & (write.token == entries[1])
        | (write.position != 2) & (write.token == entries[2])
        | (write.position != 3) & (write.token == entries[3])
        | (write.position != 4) & (write.token == entries[4])
        | (write.position != 5) & (write.token == entries[5])
        | (write.position != 6) & (write.token == entries[6])
        | (write.position != 7) & (write.token == entries[7])
        | (write.position != 8) & (write.token == entries[8])
        | (write.position != 9) & (write.token == entries[9])
        | (write.position != 10) & (write.token == entries[10])
        | (write.position != 11) & (write.token == entries[11])
        | (write.position != 12) & (write.token == entries[12])
        | (write.position != 13) & (write.token == entries[13])
        | (write.position != 14) & (write.token == entries[14])
        | (write.position != 15) & (write.token == entries[15])
    )

    @ac.rule
    def install() -> None:
        if clear_valid:
            count = 0
            duplicate = 0
            count_error = 0
        if write_valid:
            entries[write.position] = write.token
            duplicate = duplicate | duplicate_hit
        if write_count_valid:
            if write_count <= 16:
                count = write_count
                count_error = 0
            else:
                count = 0
                count_error = 1
                duplicate = 0

    install()
    return IndexStoreResult(
        token=entries[read_position],
        count=count,
        read_valid=read_valid,
        write_accepted=write_valid,
        duplicate=duplicate,
        count_error=count_error,
    )


@ac.struct
class IndexValidation:
    token: ac.u11
    position: ac.u4
    valid: ac.u1
    out_of_range: ac.u1
    inactive: ac.u1
    duplicate: ac.u1
    count_error: ac.u1


@ac.module
def IndexValidator(
    token: ac.u11,
    position: ac.u4,
    count: ac.u5,
    key_length: ac.u11,
    active: ac.u1,
    duplicate: ac.u1,
    count_error: ac.u1,
) -> IndexValidation:
    """Validate one selected token against the query's active prefix.

    Duplicate detection is performed by ``IndexStore`` over the committed
    active prefix.  This module combines that row-level status with the
    hardware-local position and logical-token bounds before issuing a KV read.
    """

    position_active = (
        (position == 0) & (count > 0)
        | (position == 1) & (count > 1)
        | (position == 2) & (count > 2)
        | (position == 3) & (count > 3)
        | (position == 4) & (count > 4)
        | (position == 5) & (count > 5)
        | (position == 6) & (count > 6)
        | (position == 7) & (count > 7)
        | (position == 8) & (count > 8)
        | (position == 9) & (count > 9)
        | (position == 10) & (count > 10)
        | (position == 11) & (count > 11)
        | (position == 12) & (count > 12)
        | (position == 13) & (count > 13)
        | (position == 14) & (count > 14)
        | (position == 15) & (count > 15)
    )
    inactive = active & ~position_active
    out_of_range = active & position_active & ~(token < key_length)
    valid = active & position_active & (token < key_length) & ~duplicate & ~count_error
    return IndexValidation(
        token=token,
        position=position,
        valid=valid,
        out_of_range=out_of_range,
        inactive=inactive,
        duplicate=duplicate,
        count_error=count_error,
    )


@ac.struct
class KVMapEntry:
    token: ac.u11
    k_row_base: ac.u32
    v_row_base: ac.u32


@ac.struct
class KVAddress:
    token: ac.u11
    k_row_base: ac.u32
    v_row_base: ac.u32
    valid: ac.u1


@ac.struct
class KVMapState:
    pending: ac.u1
    response: KVAddress


@ac.module
def KVTokenMap(
    update_valid: ac.u1,
    update: KVMapEntry,
    lookup_token: ac.u11,
    lookup_valid: ac.u1,
) -> KVAddress:
    """Map a logical KV token to the physical K/V row bases.

    ``k_row_base`` and ``v_row_base`` are byte addresses supplied by the cache
    manager.  Keeping the map separate from the gatherer allows paged or
    compacted KV layouts without changing index-list storage.
    """

    state: KVMapState = KVMapState(
        pending=0,
        response=KVAddress(token=0, k_row_base=0, v_row_base=0, valid=0),
    )
    lookup_active = lookup_valid & ~state.pending
    mapped, unused_port = ac.sync_mem_dp[KVMapEntry](
        lookup_active,
        lookup_token,
        False,
        0,
        update_valid,
        update.token,
        update,
        0x3FF,
        depth=2048,
    )

    @ac.rule
    def issue() -> None:
        if lookup_active:
            state.pending = 1

    @ac.rule(after=(issue,), guard=lambda: state.pending)
    def capture() -> None:
        state.response = KVAddress(
            token=mapped.token,
            k_row_base=mapped.k_row_base,
            v_row_base=mapped.v_row_base,
            valid=1,
        )
        state.pending = 0

    issue()
    capture()
    return state.response


@ac.struct
class GatherAddressResult:
    token: ac.u11
    position: ac.u4
    k_row_base: ac.u32
    v_row_base: ac.u32
    valid: ac.u1
    out_of_range: ac.u1
    inactive: ac.u1
    duplicate: ac.u1
    count_error: ac.u1


@ac.module
def GatherAddress(
    validation: IndexValidation,
    mapped: KVAddress,
) -> GatherAddressResult:
    """Pair a validated index with the physical K/V row bases."""

    return GatherAddressResult(
        token=validation.token,
        position=validation.position,
        k_row_base=mapped.k_row_base,
        v_row_base=mapped.v_row_base,
        valid=validation.valid & mapped.valid,
        out_of_range=validation.out_of_range,
        inactive=validation.inactive,
        duplicate=validation.duplicate,
        count_error=validation.count_error,
    )


@ac.struct
class SparseGatherResult:
    token: ac.u11
    position: ac.u4
    k_row_base: ac.u32
    v_row_base: ac.u32
    valid: ac.u1
    out_of_range: ac.u1
    inactive: ac.u1
    duplicate: ac.u1
    count_error: ac.u1
    index_write_accepted: ac.u1


@ac.module
def SparseIndexGather(
    index_write_valid: ac.u1,
    index_write: IndexWord,
    index_begin_valid: ac.u1,
    index_count_valid: ac.u1,
    index_count: ac.u5,
    index_read_valid: ac.u1,
    index_read_position: ac.u4,
    key_length: ac.u11,
    kv_update_valid: ac.u1,
    kv_update: KVMapEntry,
) -> SparseGatherResult:
    """Compose index storage, validation, map lookup and gather addressing."""

    stored = IndexStore(
        index_write_valid,
        index_write,
        index_begin_valid,
        index_count_valid,
        index_count,
        index_read_valid,
        index_read_position,
    )
    validation = IndexValidator(
        stored.token,
        index_read_position,
        stored.count,
        key_length,
        stored.read_valid,
        stored.duplicate,
        stored.count_error,
    )
    mapped = KVTokenMap(
        kv_update_valid,
        kv_update,
        stored.token,
        validation.valid,
    )
    gathered = GatherAddress(validation, mapped)
    return SparseGatherResult(
        token=gathered.token,
        position=gathered.position,
        k_row_base=gathered.k_row_base,
        v_row_base=gathered.v_row_base,
        valid=gathered.valid,
        out_of_range=gathered.out_of_range,
        inactive=gathered.inactive,
        duplicate=gathered.duplicate,
        count_error=gathered.count_error,
        index_write_accepted=stored.write_accepted,
    )
