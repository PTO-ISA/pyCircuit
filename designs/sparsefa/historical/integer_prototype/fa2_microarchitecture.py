# ruff: noqa: E741, N802, F821, F841 -- published names and compiler-owned captures.
"""Executable integer control/address skeleton for the FA2 P0 microarchitecture.

The sparse index/gather path is implemented in ``fa2_index_gather.py``.  This
file adds the hardware-owned boundaries that surround it: lane address
expansion, a bounded transfer tag slot, the controller phase register, and an
atomic row-state commit point.  Floating-point QK/softmax/PV datapaths are
intentionally represented by typed integer state until their numerical profile
is frozen.
"""

import pycircuit as ac
from fa2.fa2_index_gather import (
    IndexWord,
    KVMapEntry,
    SparseGatherResult,
    SparseIndexGather,
)


@ac.struct
class LaneAddressResult:
    token: ac.u11
    position: ac.u4
    lane: ac.u6
    k_address: ac.u32
    v_address: ac.u32
    valid: ac.u1


@ac.module
def LaneAddressExpander(
    gather: SparseGatherResult,
    lane: ac.u32,
) -> LaneAddressResult:
    """Expand one gathered K/V row base into one element address pair.

    P0 uses two-byte FP16/BF16 elements.  The row base is already a byte
    address, so lane ``d`` contributes ``2*d`` bytes to both K and V.  The
    token and list position remain attached to the request for causal masking
    and response matching.
    """

    byte_offset = lane + lane  # lane << 1, represented as a byte offset.
    return LaneAddressResult(
        token=gather.token,
        position=gather.position,
        lane=lane[0:6],
        k_address=gather.k_row_base + byte_offset,
        v_address=gather.v_row_base + byte_offset,
        valid=gather.valid,
    )


@ac.struct
class TransferDescriptor:
    operation: ac.u8
    generation: ac.u8
    tag: ac.u4
    token: ac.u11
    position: ac.u4
    tensor_kind: ac.u2  # 0=K, 1=V, 2=INDEX, 3=COUNT
    address: ac.u32
    beat_count: ac.u8


@ac.struct
class TransferResponse:
    accepted: ac.u1
    active: ac.u1
    complete: ac.u1
    stale: ac.u1
    error: ac.u1
    descriptor: TransferDescriptor


@ac.struct
class TransferState:
    active: ac.u1
    descriptor: TransferDescriptor
    beats_seen: ac.u8
    error: ac.u1
    complete: ac.u1


@ac.module
def TransferTagTable(
    issue_valid: ac.u1,
    issue: TransferDescriptor,
    response_valid: ac.u1,
    response_tag: ac.u4,
    response_generation: ac.u8,
    response_last: ac.u1,
    cancel_valid: ac.u1,
) -> TransferResponse:
    """Own one bounded in-flight transfer descriptor for the P0 baseline.

    A response is accepted only when both tag and generation match.  A
    mismatched response is consumed as stale and cannot update the active
    descriptor.  The slot is released only after the final beat or cancel.
    """

    state: TransferState = TransferState()
    accepted = issue_valid & ~state.active & ~cancel_valid
    response_match = (
        response_valid
        & state.active
        & (response_tag == state.descriptor.tag)
        & (response_generation == state.descriptor.generation)
    )
    stale = response_valid & ~response_match

    @ac.rule
    def commit() -> None:
        if cancel_valid:
            state.active = 0
            state.complete = 0
            state.error = 0
            state.beats_seen = 0
        elif accepted:
            state.active = 1
            state.descriptor = issue
            state.beats_seen = 0
            state.complete = 0
            state.error = 0
        elif response_match:
            if response_last:
                state.active = 0
                state.complete = 1
            else:
                state.beats_seen = state.beats_seen + 1

    commit()
    return TransferResponse(
        accepted=accepted,
        active=state.active,
        complete=state.complete,
        stale=stale,
        error=state.error,
        descriptor=state.descriptor,
    )


@ac.struct
class ControllerState:
    phase: ac.u4
    operation: ac.u8
    generation: ac.u8
    error_code: ac.u4


@ac.struct
class ControllerResult:
    phase: ac.u4
    operation: ac.u8
    generation: ac.u8
    error_code: ac.u4
    running: ac.u1
    completed: ac.u1
    cancelled: ac.u1
    errored: ac.u1


@ac.module
def AttentionController(
    start_valid: ac.u1,
    operation: ac.u8,
    q_ready: ac.u1,
    index_ready: ac.u1,
    kv_ready: ac.u1,
    score_ready: ac.u1,
    row_update_committed: ac.u1,
    normalize_ready: ac.u1,
    output_committed: ac.u1,
    cancel_valid: ac.u1,
    error_valid: ac.u1,
    error_code: ac.u4,
) -> ControllerResult:
    """Advance the bounded P0 phase machine and retain operation identity."""

    state: ControllerState = ControllerState()
    idle = state.phase == 0
    running = (state.phase != 0) & (state.phase != 13) & (state.phase != 14)

    @ac.rule
    def advance() -> None:
        if idle & start_valid:
            state.phase = 1
            state.operation = operation
            state.generation = state.generation + 1
            state.error_code = 0
        elif cancel_valid & running:
            state.phase = 13
        elif error_valid & running:
            state.phase = 14
            state.error_code = error_code
        elif (state.phase == 1) & q_ready:
            state.phase = 2
        elif (state.phase == 2) & index_ready:
            state.phase = 3
        elif (state.phase == 3) & kv_ready:
            state.phase = 4
        elif (state.phase == 4) & score_ready:
            state.phase = 5
        elif (state.phase == 5) & row_update_committed:
            state.phase = 6
        elif (state.phase == 6) & normalize_ready:
            state.phase = 7
        elif (state.phase == 7) & output_committed:
            state.phase = 12

    advance()
    return ControllerResult(
        phase=state.phase,
        operation=state.operation,
        generation=state.generation,
        error_code=state.error_code,
        running=running,
        completed=state.phase == 12,
        cancelled=state.phase == 13,
        errored=state.phase == 14,
    )


@ac.struct
class RowState:
    m: ac.u32
    l: ac.u32
    accumulator: ac.u32
    valid: ac.u1


@ac.module
def RowStateCommit(
    update_valid: ac.u1,
    new_m: ac.u32,
    new_l: ac.u32,
    new_accumulator: ac.u32,
    clear_valid: ac.u1,
) -> RowState:
    """Atomically replace one row's online-softmax shadow state.

    The fields are integer placeholders for the current frontend bring-up;
    the owner and all-or-nothing commit boundary are identical to the future
    FP32 ``m/l/acc`` implementation.
    """

    state: RowState = RowState(m=0, l=0, accumulator=0, valid=0)

    @ac.rule
    def commit() -> None:
        if clear_valid:
            state.m = 0
            state.l = 0
            state.accumulator = 0
            state.valid = 0
        elif update_valid:
            state.m = new_m
            state.l = new_l
            state.accumulator = new_accumulator
            state.valid = 1

    commit()
    return state


@ac.struct
class MicroarchitectureResult:
    lane: LaneAddressResult
    transfer: TransferResponse
    controller: ControllerResult
    row: RowState


@ac.module
def FlashAttention2Microarchitecture(
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
    lane: ac.u32,
    transfer_issue_valid: ac.u1,
    transfer_issue: TransferDescriptor,
    transfer_response_valid: ac.u1,
    transfer_response_tag: ac.u4,
    transfer_response_generation: ac.u8,
    transfer_response_last: ac.u1,
    transfer_cancel_valid: ac.u1,
    start_valid: ac.u1,
    operation: ac.u8,
    q_ready: ac.u1,
    index_ready: ac.u1,
    kv_ready: ac.u1,
    score_ready: ac.u1,
    row_update_committed: ac.u1,
    normalize_ready: ac.u1,
    output_committed: ac.u1,
    cancel_valid: ac.u1,
    error_valid: ac.u1,
    error_code: ac.u4,
    row_update_valid: ac.u1,
    row_new_m: ac.u32,
    row_new_l: ac.u32,
    row_new_accumulator: ac.u32,
    row_clear_valid: ac.u1,
) -> MicroarchitectureResult:
    """Top-level executable composition of the P0 control/address modules."""

    gathered = SparseIndexGather(
        index_write_valid,
        index_write,
        index_begin_valid,
        index_count_valid,
        index_count,
        index_read_valid,
        index_read_position,
        key_length,
        kv_update_valid,
        kv_update,
    )
    lane_result = LaneAddressExpander(gathered, lane)
    transfer_result = TransferTagTable(
        transfer_issue_valid,
        transfer_issue,
        transfer_response_valid,
        transfer_response_tag,
        transfer_response_generation,
        transfer_response_last,
        transfer_cancel_valid,
    )
    controller_result = AttentionController(
        start_valid,
        operation,
        q_ready,
        index_ready,
        kv_ready,
        score_ready,
        row_update_committed,
        normalize_ready,
        output_committed,
        cancel_valid,
        error_valid,
        error_code,
    )
    row_result = RowStateCommit(
        row_update_valid,
        row_new_m,
        row_new_l,
        row_new_accumulator,
        row_clear_valid,
    )
    return MicroarchitectureResult(
        lane=lane_result,
        transfer=transfer_result,
        controller=controller_result,
        row=row_result,
    )
