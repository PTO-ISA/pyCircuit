# ruff: noqa: F821, F823, F841, N802
"""Full B/H/query scheduler for the serial FA2 row engine.

Explicit byte strides describe row-major regions; inputs may broadcast with
zero strides, while output rows/heads/batches must remain disjoint. The caller
owns allocation bounds and must supply distinct writable O and immutable input
regions. This hardware checks alignment, layout extents and 64-bit overflow.
One memory request is outstanding globally. Cancellation drains that request
before terminal publication; output is valid only on successful completion.
"""
import pycircuit as ac
from fa2.fa2_engine import Extend7, Extend32, Extend64, FA2Engine
from fa2.fa2_interfaces import EngineCommand, EngineStatus, MemoryReply, MemoryRequest


@ac.struct
class TensorCommand:
    valid: ac.u1
    tag: ac.u32
    batches: ac.u16
    heads: ac.u16
    queries: ac.u32
    keys: ac.u32
    dimensions: ac.u7
    scale: ac.u32
    indexed: ac.u1
    causal: ac.u1
    bf16: ac.u1
    causal_origin: ac.u32
    causal_origin_negative: ac.u1
    q_base: ac.u64
    k_base: ac.u64
    v_base: ac.u64
    o_base: ac.u64
    count_base: ac.u64
    index_base: ac.u64
    q_batch_stride: ac.u64
    q_head_stride: ac.u64
    q_row_stride: ac.u64
    k_batch_stride: ac.u64
    k_head_stride: ac.u64
    k_row_stride: ac.u64
    v_batch_stride: ac.u64
    v_head_stride: ac.u64
    v_row_stride: ac.u64
    o_batch_stride: ac.u64
    o_head_stride: ac.u64
    o_row_stride: ac.u64
    count_batch_stride: ac.u64
    count_head_stride: ac.u64
    count_row_stride: ac.u64
    index_batch_stride: ac.u64
    index_head_stride: ac.u64
    index_row_stride: ac.u64


@ac.module
def SchedulerWide16(value: ac.u16) -> ac.u64:
    return value


@ac.module
def TensorLayoutValid(
    base: ac.u64,
    batch_stride: ac.u64,
    head_stride: ac.u64,
    row_stride: ac.u64,
    batches: ac.u16,
    heads: ac.u16,
    rows: ac.u32,
    width: ac.u64,
    metadata: ac.u1,
    broadcast: ac.u1,
) -> ac.u1:
    # Last-address checks use wide arithmetic, including each multiplication.
    last_row = rows - 1 if rows != 0 else 0
    last_head = heads - 1 if heads != 0 else 0
    last_batch = batches - 1 if batches != 0 else 0
    row_extent = Extend64(Extend32(last_row)) * Extend64(row_stride) + Extend64(width)
    head_extent = (
        Extend64(SchedulerWide16(last_head)) * Extend64(head_stride) + row_extent
    )
    tensor_extent = (
        Extend64(SchedulerWide16(last_batch)) * Extend64(batch_stride) + head_extent
    )
    end = Extend64(base) + tensor_extent
    alignment_mask = 3 if metadata else 1
    aligned = ((base | batch_stride | head_stride | row_stride) & alignment_mask) == 0
    separate_rows = (
        (rows <= 1) | (row_stride >= width) | (broadcast & (row_stride == 0))
    )
    separate_heads = (
        (heads <= 1)
        | (Extend64(head_stride) >= row_extent)
        | (broadcast & (head_stride == 0))
    )
    separate_batches = (
        (batches <= 1)
        | (Extend64(batch_stride) >= head_extent)
        | (broadcast & (batch_stride == 0))
    )
    return (
        aligned
        & separate_rows
        & separate_heads
        & separate_batches
        & (end <= 18446744073709551616)
    )


@ac.module
def FA2TensorScheduler(
    command: TensorCommand, memory: MemoryReply, completion_ready: ac.u1, cancel: ac.u1
) -> tuple[MemoryRequest, EngineStatus]:
    # phase0 idle, phase1 row offer, phase2 row execution/drain, phase3 terminal.
    saved: TensorCommand = TensorCommand()
    phase: ac.u5 = 0
    batch: ac.u16 = 0
    head: ac.u16 = 0
    query: ac.u32 = 0
    code: ac.u4 = 0
    cancel_pending: ac.u1 = 0
    batch_offset = SchedulerWide16(batch)
    head_offset = SchedulerWide16(head)
    query_offset = Extend32(query)
    row_q = (
        saved.q_base
        + batch_offset * saved.q_batch_stride
        + head_offset * saved.q_head_stride
        + query_offset * saved.q_row_stride
    )
    row_k = (
        saved.k_base
        + batch_offset * saved.k_batch_stride
        + head_offset * saved.k_head_stride
    )
    row_v = (
        saved.v_base
        + batch_offset * saved.v_batch_stride
        + head_offset * saved.v_head_stride
    )
    row_o = (
        saved.o_base
        + batch_offset * saved.o_batch_stride
        + head_offset * saved.o_head_stride
        + query_offset * saved.o_row_stride
    )
    row_count = (
        saved.count_base
        + batch_offset * saved.count_batch_stride
        + head_offset * saved.count_head_stride
        + query_offset * saved.count_row_stride
    )
    row_index = (
        saved.index_base
        + batch_offset * saved.index_batch_stride
        + head_offset * saved.index_head_stride
        + query_offset * saved.index_row_stride
    )
    causal_empty = saved.causal_origin_negative & (query < saved.causal_origin)
    causal_position = (
        query - saved.causal_origin
        if saved.causal_origin_negative
        else query + saved.causal_origin
    )
    child_command = EngineCommand(
        valid=phase == 1,
        tag=saved.tag,
        q_base=row_q,
        k_base=row_k,
        v_base=row_v,
        o_base=row_o,
        count_base=row_count,
        index_base=row_index,
        k_stride=saved.k_row_stride,
        v_stride=saved.v_row_stride,
        key_length=saved.keys,
        query_position=causal_position,
        dimensions=saved.dimensions,
        scale=saved.scale,
        indexed=saved.indexed,
        causal=saved.causal,
        causal_empty=causal_empty,
        bf16=saved.bf16,
    )
    request, child_status = FA2Engine(
        child_command, memory, phase == 2, cancel | cancel_pending
    )
    # Scalar wire snapshots keep each Rule's captures limited to its field uses.
    saved_queries = saved.queries
    saved_heads = saved.heads
    saved_batches = saved.batches
    child_command_ready = child_status.command_ready
    child_completion_valid = child_status.completion_valid
    child_code = child_status.code
    width = Extend7(command.dimensions) * 2
    q_layout = TensorLayoutValid(
        command.q_base,
        command.q_batch_stride,
        command.q_head_stride,
        command.q_row_stride,
        command.batches,
        command.heads,
        command.queries,
        width,
        0,
        1,
    )
    k_layout = TensorLayoutValid(
        command.k_base,
        command.k_batch_stride,
        command.k_head_stride,
        command.k_row_stride,
        command.batches,
        command.heads,
        command.keys,
        width,
        0,
        1,
    )
    v_layout = TensorLayoutValid(
        command.v_base,
        command.v_batch_stride,
        command.v_head_stride,
        command.v_row_stride,
        command.batches,
        command.heads,
        command.keys,
        width,
        0,
        1,
    )
    o_layout = TensorLayoutValid(
        command.o_base,
        command.o_batch_stride,
        command.o_head_stride,
        command.o_row_stride,
        command.batches,
        command.heads,
        command.queries,
        width,
        0,
        0,
    )
    count_layout = TensorLayoutValid(
        command.count_base,
        command.count_batch_stride,
        command.count_head_stride,
        command.count_row_stride,
        command.batches,
        command.heads,
        command.queries,
        4,
        1,
        1,
    )
    index_layout = TensorLayoutValid(
        command.index_base,
        command.index_batch_stride,
        command.index_head_stride,
        command.index_row_stride,
        command.batches,
        command.heads,
        command.queries,
        8192,
        1,
        1,
    )
    causal_end = Extend32(command.queries) + Extend32(command.causal_origin)
    causal_bad = (
        command.causal & ~command.causal_origin_negative & (causal_end > 4294967296)
    )
    layout_bad = (
        ~q_layout
        | ~k_layout
        | ~v_layout
        | ~o_layout
        | (command.indexed & (~count_layout | ~index_layout))
    )

    scale_bad = (command.scale < 8388608) | (command.scale > 1065353216)
    dense_bad = ~command.indexed & (command.keys > 2048)

    @ac.rule(guard=lambda: (phase == 0) & command.valid & ~cancel)
    def accept() -> None:
        saved = command
        batch = 0
        head = 0
        query = 0
        cancel_pending = 0
        code = 0
        phase = 1
        if (
            (command.batches == 0)
            | (command.heads == 0)
            | (command.queries == 0)
            | (command.dimensions == 0)
            | (command.dimensions > 64)
            | layout_bad
            | causal_bad
            | scale_bad
            | dense_bad
        ):
            code = 1
            phase = 3

    @ac.rule(guard=lambda: (phase == 1) & child_command_ready & ~cancel)
    def row_accept() -> None:
        phase = 2

    @ac.rule(guard=lambda: (phase == 2) & child_completion_valid)
    def row_complete() -> None:
        if cancel | cancel_pending:
            code = 6
            phase = 3
        elif child_code != 0:
            code = child_code
            phase = 3
        elif query == saved_queries - 1:
            query = 0
            if head == saved_heads - 1:
                head = 0
                if batch == saved_batches - 1:
                    phase = 3
                else:
                    batch = batch + 1
                    phase = 1
            else:
                head = head + 1
                phase = 1
        else:
            query = query + 1
            phase = 1

    @ac.rule(guard=lambda: (phase == 1) & cancel)
    def cancel_offer() -> None:
        code = 6
        phase = 3

    @ac.rule(guard=lambda: (phase == 2) & cancel & ~child_completion_valid)
    def cancel_active() -> None:
        cancel_pending = 1

    @ac.rule(guard=lambda: (phase == 3) & completion_ready)
    def retire() -> None:
        phase = 0
        cancel_pending = 0

    accept()
    row_accept()
    row_complete()
    cancel_offer()
    cancel_active()
    retire()
    return request, EngineStatus(
        command_ready=(phase == 0) & ~cancel,
        completion_valid=phase == 3,
        tag=saved.tag,
        code=code,
        phase=phase,
    )
