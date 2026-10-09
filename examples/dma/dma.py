"""Two read-first memories connected by complete-packet DMA queues."""

# ruff: noqa: N802 -- hardware definition names.

import pycircuit as ac


@ac.struct
class DmaRequest:
    dram_address: ac.u4
    sram_address: ac.u4
    data: ac.u16
    tag: ac.u8


@ac.struct
class DmaResult:
    seed_ready: ac.u1
    copy_ready: ac.u1
    check_ready: ac.u1
    seed_valid: ac.u1
    copy_valid: ac.u1
    check_valid: ac.u1
    dram_accepted: ac.u2
    dram_enqueued: ac.u2
    sram_accepted: ac.u2
    sram_enqueued: ac.u2
    seed_response: DmaRequest
    copy_response: DmaRequest
    check_response: DmaRequest


@ac.struct
class MemoryState:
    busy: ac.u1
    capture_old: ac.u1
    remaining: ac.u2
    reader: ac.u1
    request: DmaRequest


@ac.struct
class MemoryResult:
    reader_ready: ac.u1
    writer_ready: ac.u1
    reader_valid: ac.u1
    writer_valid: ac.u1
    accepted: ac.u2
    enqueued: ac.u2
    reader_response: DmaRequest
    writer_response: DmaRequest


@ac.rule
def Advance(state, accepted, request, reader, raw_data, release, initial_delay):
    # All branches read old busy; releasing cannot admit another request.
    if state.busy:
        if state.capture_old:
            state.request.data = raw_data
            state.capture_old = 0
        if state.remaining > 1:
            state.remaining = state.remaining - 1
        if release:
            state.busy = 0
    else:
        if accepted:
            state.request = request
            state.reader = reader
            state.busy = 1
            state.capture_old = 1
            state.remaining = initial_delay


@ac.module
def Dram(
    copy_valid: ac.u1,
    copy: DmaRequest,
    seed_valid: ac.u1,
    seed: DmaRequest,
    take_copy: ac.u1,
    take_seed: ac.u1,
) -> MemoryResult:
    state: MemoryState = MemoryState()
    idle = ~state.busy
    reader_ready = idle & copy_valid
    writer_ready = idle & ~copy_valid
    accepted_reader = copy_valid & reader_ready
    accepted_writer = seed_valid & writer_ready
    accepted = accepted_reader | accepted_writer
    selected = copy if copy_valid else seed
    raw = ac.sync_mem[ac.u16](
        accepted,
        selected.dram_address,
        accepted_writer,
        selected.dram_address,
        selected.data,
        3,
        depth=16,
    )
    eligible = state.busy & (state.remaining == 1)
    old_data = raw if state.capture_old else state.request.data
    response = state.request
    response.data = old_data
    eligible_reader = eligible & state.reader
    eligible_writer = eligible & ~state.reader
    reader_space, reader_valid, reader_head = ac.queue[DmaRequest](
        eligible_reader, response, take_copy, depth=2
    )
    writer_space, writer_valid, writer_head = ac.queue[DmaRequest](
        eligible_writer, response, take_seed, depth=2
    )
    enqueued_reader = eligible_reader & reader_space
    enqueued_writer = eligible_writer & writer_space
    release = enqueued_reader | enqueued_writer
    Advance(state, accepted, selected, copy_valid, raw, release, 3)
    return MemoryResult(
        reader_ready=reader_ready,
        writer_ready=writer_ready,
        reader_valid=reader_valid,
        writer_valid=writer_valid,
        accepted=ac.concat(accepted_writer, accepted_reader),
        enqueued=ac.concat(enqueued_writer, enqueued_reader),
        reader_response=reader_head,
        writer_response=writer_head,
    )


@ac.module
def Sram(
    check_valid: ac.u1,
    check: DmaRequest,
    copy_valid: ac.u1,
    copy: DmaRequest,
    take_check: ac.u1,
    take_copy: ac.u1,
) -> MemoryResult:
    state: MemoryState = MemoryState()
    idle = ~state.busy
    reader_ready = idle & check_valid
    writer_ready = idle & ~check_valid
    accepted_reader = check_valid & reader_ready
    accepted_writer = copy_valid & writer_ready
    accepted = accepted_reader | accepted_writer
    selected = check if check_valid else copy
    raw = ac.sync_mem[ac.u16](
        accepted,
        selected.sram_address,
        accepted_writer,
        selected.sram_address,
        selected.data,
        3,
        depth=16,
    )
    eligible = state.busy & (state.remaining == 1)
    old_data = raw if state.capture_old else state.request.data
    response = state.request
    response.data = old_data
    eligible_reader = eligible & state.reader
    eligible_writer = eligible & ~state.reader
    reader_space, reader_valid, reader_head = ac.queue[DmaRequest](
        eligible_reader, response, take_check, depth=2
    )
    writer_space, writer_valid, writer_head = ac.queue[DmaRequest](
        eligible_writer, response, take_copy, depth=2
    )
    enqueued_reader = eligible_reader & reader_space
    enqueued_writer = eligible_writer & writer_space
    release = enqueued_reader | enqueued_writer
    Advance(state, accepted, selected, check_valid, raw, release, 2)
    return MemoryResult(
        reader_ready=reader_ready,
        writer_ready=writer_ready,
        reader_valid=reader_valid,
        writer_valid=writer_valid,
        accepted=ac.concat(accepted_writer, accepted_reader),
        enqueued=ac.concat(enqueued_writer, enqueued_reader),
        reader_response=reader_head,
        writer_response=writer_head,
    )


@ac.module
def Dma(
    seed_valid: ac.u1,
    seed: DmaRequest,
    copy_valid: ac.u1,
    copy: DmaRequest,
    check_valid: ac.u1,
    check: DmaRequest,
    take_seed: ac.u1,
    take_copy: ac.u1,
    take_check: ac.u1,
) -> DmaResult:
    seed_ready, seed_pending, seed_head = ac.queue[DmaRequest](
        seed_valid,
        seed,
        dram.writer_ready,  # noqa: F821 - forward module connection
        depth=2,
    )  # noqa: F821 -- forward module connection
    copy_ready, copy_pending, copy_head = ac.queue[DmaRequest](
        copy_valid,
        copy,
        dram.reader_ready,  # noqa: F821 - forward module connection
        depth=4,
    )  # noqa: F821 -- forward module connection
    check_ready, check_pending, check_head = ac.queue[DmaRequest](
        check_valid,
        check,
        sram.reader_ready,  # noqa: F821 - forward module connection
        depth=2,
    )  # noqa: F821 -- forward module connection
    dram = Dram(
        copy_pending,
        copy_head,
        seed_pending,
        seed_head,
        sram.writer_ready,  # noqa: F821 - forward module connection
        take_seed,
    )  # noqa: F821 -- forward module connection
    sram = Sram(
        check_pending,
        check_head,
        dram.reader_valid,
        dram.reader_response,
        take_check,
        take_copy,
    )
    return DmaResult(
        seed_ready=seed_ready,
        copy_ready=copy_ready,
        check_ready=check_ready,
        seed_valid=dram.writer_valid,
        copy_valid=sram.writer_valid,
        check_valid=sram.reader_valid,
        dram_accepted=dram.accepted,
        dram_enqueued=dram.enqueued,
        sram_accepted=sram.accepted,
        sram_enqueued=sram.enqueued,
        seed_response=dram.writer_response,
        copy_response=sram.writer_response,
        check_response=sram.reader_response,
    )
