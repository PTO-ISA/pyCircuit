"""Historical memory protocol fixtures over ordinary modules, rules and queues."""
# ruff: noqa: N802 -- hardware definition names.

import pycircuit as ac


@ac.struct
class Request:
    address: ac.u4
    write: ac.u1
    data: ac.u16
    tag: ac.u8


@ac.struct
class ReadRequest:
    address: ac.u4
    data: ac.u16
    tag: ac.u8


@ac.struct
class ControllerState:
    busy: ac.u1
    capture_old: ac.u1
    remaining: ac.u2
    request: Request


@ac.struct
class ControllerResult:
    request_ready: ac.u1
    response_valid: ac.u1
    accepted: ac.u1
    response_enqueued: ac.u1
    response: Request


@ac.struct
class ProtocolResult:
    input_ready: ac.u1
    output_valid: ac.u1
    accepted: ac.u1
    response_enqueued: ac.u1
    response: Request


@ac.struct
class ReadProtocolResult:
    input_ready: ac.u1
    output_valid: ac.u1
    accepted: ac.u1
    response_enqueued: ac.u1
    response: ReadRequest


@ac.rule
def Advance(state, accepted, request, raw_data, release, initial_delay):
    # Branch on old busy; a release never accepts a new request on its edge.
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
            state.busy = 1
            state.capture_old = 1
            state.remaining = initial_delay


@ac.module
def ControllerL1(valid: ac.u1, request: Request, take: ac.u1) -> ControllerResult:
    state: ControllerState = ControllerState()
    request_ready = ~state.busy
    accepted = valid & request_ready
    raw = ac.sync_mem[ac.u16](accepted, request.address,
                            accepted & request.write, request.address,
                            request.data, 3, depth=16)
    eligible = state.busy & (state.remaining == 1)
    old_data = raw if state.capture_old else state.request.data
    response = state.request
    response.data = old_data
    response_space, response_valid, response_head = ac.queue[Request](
        eligible, response, take, depth=1)
    release = eligible & response_space
    Advance(state, accepted, request, raw, release, 1)
    return ControllerResult(request_ready=request_ready,
                            response_valid=response_valid, accepted=accepted,
                            response_enqueued=release, response=response_head)


@ac.module
def ControllerL3(valid: ac.u1, request: Request, take: ac.u1) -> ControllerResult:
    state: ControllerState = ControllerState()
    request_ready = ~state.busy
    accepted = valid & request_ready
    raw = ac.sync_mem[ac.u16](accepted, request.address,
                            accepted & request.write, request.address,
                            request.data, 3, depth=16)
    eligible = state.busy & (state.remaining == 1)
    old_data = raw if state.capture_old else state.request.data
    response = state.request
    response.data = old_data
    response_space, response_valid, response_head = ac.queue[Request](
        eligible, response, take, depth=1)
    release = eligible & response_space
    Advance(state, accepted, request, raw, release, 3)
    return ControllerResult(request_ready=request_ready,
                            response_valid=response_valid, accepted=accepted,
                            response_enqueued=release, response=response_head)


@ac.module
def ControllerL3Buffered(valid: ac.u1, request: Request, take: ac.u1) -> ControllerResult:
    state: ControllerState = ControllerState()
    request_ready = ~state.busy
    accepted = valid & request_ready
    raw = ac.sync_mem[ac.u16](accepted, request.address,
                            accepted & request.write, request.address,
                            request.data, 3, depth=16)
    eligible = state.busy & (state.remaining == 1)
    old_data = raw if state.capture_old else state.request.data
    response = state.request
    response.data = old_data
    response_space, response_valid, response_head = ac.queue[Request](
        eligible, response, take, depth=4)
    release = eligible & response_space
    Advance(state, accepted, request, raw, release, 3)
    return ControllerResult(request_ready=request_ready,
                            response_valid=response_valid, accepted=accepted,
                            response_enqueued=release, response=response_head)


@ac.module
def MemoryBusy(valid: ac.u1, request: ReadRequest, take: ac.u1) -> ReadProtocolResult:
    input_ready, request_valid, request_head = ac.queue[ReadRequest](
        valid, request, controller.request_ready, depth=4)  # noqa: F821 - MLIR resolves forward module fields
    packet = Request(address=request_head.address, write=0,
                     data=request_head.data, tag=request_head.tag)
    controller = ControllerL3(request_valid, packet, take)
    response = ReadRequest(address=controller.response.address,
                           data=controller.response.data, tag=controller.response.tag)
    return ReadProtocolResult(input_ready=input_ready,
                              output_valid=controller.response_valid,
                              accepted=controller.accepted,
                              response_enqueued=controller.response_enqueued,
                              response=response)


@ac.module
def MemoryPipeline(valid: ac.u1, request: Request, take: ac.u1) -> ProtocolResult:
    input_ready, request_valid, request_head = ac.queue[Request](
        valid, request, controller.request_ready, depth=4)  # noqa: F821 - MLIR resolves forward module fields
    controller = ControllerL3Buffered(request_valid, request_head, take)
    return ProtocolResult(input_ready=input_ready,
                          output_valid=controller.response_valid,
                          accepted=controller.accepted,
                          response_enqueued=controller.response_enqueued,
                          response=controller.response)


@ac.struct
class SharedControllerResult:
    request_ready_writer: ac.u1
    request_ready_reader: ac.u1
    response_valid_writer: ac.u1
    response_valid_reader: ac.u1
    accepted_writer: ac.u1
    accepted_reader: ac.u1
    response_enqueued_writer: ac.u1
    response_enqueued_reader: ac.u1
    response_writer: ReadRequest
    response_reader: ReadRequest


@ac.struct
class SharedProtocolResult:
    input_ready_writer: ac.u1
    input_ready_reader: ac.u1
    output_valid_writer: ac.u1
    output_valid_reader: ac.u1
    accepted_writer: ac.u1
    accepted_reader: ac.u1
    response_enqueued_writer: ac.u1
    response_enqueued_reader: ac.u1
    response_writer: ReadRequest
    response_reader: ReadRequest


@ac.module
def SharedControllerL1(valid_writer: ac.u1, writer: ReadRequest,
                       valid_reader: ac.u1, reader: ReadRequest,
                       take_writer: ac.u1, take_reader: ac.u1) -> SharedControllerResult:
    state: ControllerState = ControllerState()
    idle = ~state.busy
    # One selected endpoint is ready: writer whenever it has a valid request.
    request_ready_writer = idle & valid_writer
    request_ready_reader = idle & ~valid_writer
    accepted_writer = valid_writer & request_ready_writer
    accepted_reader = valid_reader & request_ready_reader
    accepted = accepted_writer | accepted_reader
    selected = writer if valid_writer else reader
    packet = Request(address=selected.address, write=valid_writer,
                     data=selected.data, tag=selected.tag)
    raw = ac.sync_mem[ac.u16](accepted, packet.address,
                            accepted & packet.write, packet.address,
                            packet.data, 3, depth=16)
    eligible = state.busy & (state.remaining == 1)
    old_data = raw if state.capture_old else state.request.data
    response = ReadRequest(address=state.request.address,
                           data=old_data, tag=state.request.tag)
    # The saved endpoint controls routing and release even if request heads change.
    eligible_writer = eligible & state.request.write
    eligible_reader = eligible & ~state.request.write
    writer_space, writer_valid, writer_head = ac.queue[ReadRequest](
        eligible_writer, response, take_writer, depth=2)
    reader_space, reader_valid, reader_head = ac.queue[ReadRequest](
        eligible_reader, response, take_reader, depth=2)
    enqueued_writer = eligible_writer & writer_space
    enqueued_reader = eligible_reader & reader_space
    release = enqueued_writer | enqueued_reader
    Advance(state, accepted, packet, raw, release, 1)
    return SharedControllerResult(request_ready_writer=request_ready_writer,
                                  request_ready_reader=request_ready_reader,
                                  response_valid_writer=writer_valid,
                                  response_valid_reader=reader_valid,
                                  accepted_writer=accepted_writer,
                                  accepted_reader=accepted_reader,
                                  response_enqueued_writer=enqueued_writer,
                                  response_enqueued_reader=enqueued_reader,
                                  response_writer=writer_head,
                                  response_reader=reader_head)


@ac.module
def MemorySimple(valid_writer: ac.u1, writer: ReadRequest,
                  valid_reader: ac.u1, reader: ReadRequest,
                  take_writer: ac.u1, take_reader: ac.u1) -> SharedProtocolResult:
    input_ready_writer, writer_valid, writer_head = ac.queue[ReadRequest](
        valid_writer, writer, controller.request_ready_writer, depth=4)  # noqa: F821 - forward connection
    input_ready_reader, reader_valid, reader_head = ac.queue[ReadRequest](
        valid_reader, reader, controller.request_ready_reader, depth=4)  # noqa: F821 - forward connection
    controller = SharedControllerL1(writer_valid, writer_head,
                                    reader_valid, reader_head,
                                    take_writer, take_reader)
    return SharedProtocolResult(input_ready_writer=input_ready_writer,
                                input_ready_reader=input_ready_reader,
                                output_valid_writer=controller.response_valid_writer,
                                output_valid_reader=controller.response_valid_reader,
                                accepted_writer=controller.accepted_writer,
                                accepted_reader=controller.accepted_reader,
                                response_enqueued_writer=controller.response_enqueued_writer,
                                response_enqueued_reader=controller.response_enqueued_reader,
                                response_writer=controller.response_writer,
                                response_reader=controller.response_reader)
