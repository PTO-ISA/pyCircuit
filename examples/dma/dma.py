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


@ac.struct
class RequestChannel:
    valid: ac.u1
    data: DmaRequest


@ac.module
def Dram(
    copy: RequestChannel,
    seed: RequestChannel,
    take_seed: ac.u1,
) -> MemoryResult:
    state: MemoryState = MemoryState()
    busy = state.busy
    remaining = state.remaining
    reader = state.reader
    idle = ~busy
    reader_ready = idle & copy.valid
    writer_ready = idle & ~copy.valid
    accepted_reader = copy.valid & reader_ready
    accepted_writer = seed.valid & writer_ready
    accepted = accepted_reader | accepted_writer
    selected = copy.data if copy.valid else seed.data
    memory = ac.sync_mem[ac.u16](depth=16)
    eligible = busy & (remaining == 1)

    @ac.rule
    def current_response() -> DmaRequest:
        # The public response includes the RAM snapshot before reader_valid.
        response = state.request
        if state.capture_old:
            response.data = memory.result()
        return response

    response = current_response()
    writer_responses = ac.queue[DmaRequest](
        depth=2, ready_policy="local_occupancy", out_ready=take_seed
    )

    @ac.rule(guard=lambda: busy & (remaining > 1))
    def advance_latency() -> None:
        if state.capture_old:
            state.request.data = memory.result()
            state.capture_old = 0
        state.remaining = state.remaining - 1

    @ac.rule(guard=lambda: eligible & reader)
    def finish_read() -> DmaRequest:
        state.busy = 0
        return response

    @ac.rule(guard=lambda: eligible & ~reader)
    def finish_write() -> None:
        writer_responses.push(response)
        state.busy = 0

    @ac.rule(guard=lambda: accepted)
    def accept_request() -> None:
        # Admission uses the saved old idle state: release cannot accept again.
        memory.request(selected.dram_address)
        if accepted_writer:
            memory.write(selected.dram_address, selected.data)
        state.request = selected
        state.reader = copy.valid
        state.busy = 1
        state.capture_old = 1
        state.remaining = 3

    advance_latency()
    finish_write()
    accept_request()
    return MemoryResult(
        reader_ready=reader_ready,
        writer_ready=writer_ready,
        reader_valid=eligible & state.reader,
        writer_valid=writer_responses.valid,
        accepted=ac.concat(accepted_writer, accepted_reader),
        enqueued=ac.concat(
            eligible & ~state.reader & writer_responses.ready,
            eligible & state.reader,
        ),
        reader_response=response,
        writer_response=writer_responses.data,
    )


@ac.module
def Sram(
    check: RequestChannel,
    take_check: ac.u1,
    take_copy: ac.u1,
) -> MemoryResult:
    state: MemoryState = MemoryState()
    busy = state.busy
    remaining = state.remaining
    reader = state.reader
    idle = ~busy
    reader_ready = idle & check.valid
    copy_requests = ac.queue[DmaRequest](depth=2, ready_policy="local_occupancy")
    writer_ready = copy_requests.ready
    accepted_reader = check.valid & reader_ready
    accepted_writer = copy_requests.valid & idle & ~check.valid
    accepted = accepted_reader | accepted_writer
    selected = check.data if check.valid else copy_requests.data
    memory = ac.sync_mem[ac.u16](depth=16)
    eligible = busy & (remaining == 1)
    response = state.request
    reader_responses = ac.queue[DmaRequest](
        depth=2, ready_policy="local_occupancy", out_ready=take_check
    )
    writer_responses = ac.queue[DmaRequest](
        depth=2, ready_policy="local_occupancy", out_ready=take_copy
    )

    @ac.rule
    def enqueue_copy(request: DmaRequest) -> None:
        copy_requests.push(request)

    @ac.rule(guard=lambda: busy & (remaining > 1))
    def advance_latency() -> None:
        if state.capture_old:
            state.request.data = memory.result()
            state.capture_old = 0
        state.remaining = state.remaining - 1

    @ac.rule(guard=lambda: eligible & reader)
    def finish_read() -> None:
        reader_responses.push(response)
        state.busy = 0

    @ac.rule(guard=lambda: eligible & ~reader)
    def finish_write() -> None:
        writer_responses.push(response)
        state.busy = 0

    @ac.rule(guard=lambda: accepted)
    def accept_request() -> None:
        # Admission uses the saved old idle state: release cannot accept again.
        if ~check.valid:
            copy_requests.pop()
        memory.request(selected.sram_address)
        if accepted_writer:
            memory.write(selected.sram_address, selected.data)
        state.request = selected
        state.reader = check.valid
        state.busy = 1
        state.capture_old = 1
        state.remaining = 2

    advance_latency()
    finish_read()
    finish_write()
    accept_request()
    return MemoryResult(
        reader_ready=reader_ready,
        writer_ready=writer_ready,
        reader_valid=reader_responses.valid,
        writer_valid=writer_responses.valid,
        accepted=ac.concat(accepted_writer, accepted_reader),
        enqueued=ac.concat(
            eligible & ~state.reader & writer_responses.ready,
            eligible & state.reader & reader_responses.ready,
        ),
        reader_response=reader_responses.data,
        writer_response=writer_responses.data,
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
    seed_requests = ac.queue[DmaRequest](
        in_valid=seed_valid,
        in_data=seed,
        out_ready=dram.writer_ready,  # noqa: F821 - forward child connection
        depth=2,
        ready_policy="local_occupancy",
    )
    copy_requests = ac.queue[DmaRequest](
        in_valid=copy_valid,
        in_data=copy,
        out_ready=dram.reader_ready,  # noqa: F821 - forward child connection
        depth=4,
        ready_policy="local_occupancy",
    )
    check_requests = ac.queue[DmaRequest](
        in_valid=check_valid,
        in_data=check,
        out_ready=sram.reader_ready,  # noqa: F821 - forward child connection
        depth=2,
        ready_policy="local_occupancy",
    )
    dram = Dram(
        RequestChannel(valid=copy_requests.valid, data=copy_requests.data),
        RequestChannel(valid=seed_requests.valid, data=seed_requests.data),
        take_seed,
    )  # noqa: F821 -- forward module connection
    sram = Sram(
        RequestChannel(valid=check_requests.valid, data=check_requests.data),
        take_check,
        take_copy,
    )

    @ac.rule
    def forward_copy() -> None:
        sram.enqueue_copy(dram.finish_read())

    forward_copy()
    return DmaResult(
        seed_ready=seed_requests.ready,
        copy_ready=copy_requests.ready,
        check_ready=check_requests.ready,
        seed_valid=dram.writer_valid,
        copy_valid=sram.writer_valid,
        check_valid=sram.reader_valid,
        dram_accepted=dram.accepted,
        dram_enqueued=ac.concat(
            dram.enqueued[1:2], dram.reader_valid & sram.writer_ready
        ),
        sram_accepted=sram.accepted,
        sram_enqueued=sram.enqueued,
        seed_response=dram.writer_response,
        copy_response=sram.writer_response,
        check_response=sram.reader_response,
    )
