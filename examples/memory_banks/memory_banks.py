"""Four independent memories with head-only routing and priority merging."""

# ruff: noqa: F821, N802 -- forward hardware connections and definition names.

import pycircuit as ac


@ac.struct
class BankRequest:
    bank: ac.u2
    offset: ac.u4
    write: ac.u1
    data: ac.u16
    tag: ac.u8


@ac.struct
class BanksResult:
    input_ready: ac.u1
    output_valid: ac.u1
    response: BankRequest
    route: ac.u4
    accepted: ac.u4
    enqueued: ac.u4
    response_valid: ac.u4
    merged: ac.u4


@ac.struct
class BankState:
    busy: ac.u1
    capture_old: ac.u1
    remaining: ac.u2
    request: BankRequest


@ac.struct
class BankResult:
    input_ready: ac.u1
    output_valid: ac.u1
    response: BankRequest
    accepted: ac.u1
    enqueued: ac.u1


@ac.module
def Bank() -> BankResult:
    state: BankState = BankState()
    requests = ac.queue[BankRequest](depth=2, ready_policy="local_occupancy")
    responses = ac.queue[BankRequest](depth=2, ready_policy="local_occupancy")
    # Each transaction reads the same committed state. Admission deliberately
    # waits until the edge after a completion, preserving the release bubble.
    busy = state.busy
    capture_old = state.capture_old
    remaining = state.remaining
    idle = ~busy
    accepted = requests.valid & idle
    memory = ac.sync_mem[ac.u16](depth=16)
    eligible = busy & (remaining == 1)
    response = state.request

    @ac.rule
    def enqueue(request: BankRequest) -> None:
        requests.push(request)

    @ac.rule
    def dequeue() -> BankRequest:
        return responses.pop()

    @ac.rule
    def step() -> None:
        if idle:
            request = requests.pop()
            memory.request(request.offset)
            if request.write:
                memory.write(request.offset, request.data)
            state.request = request
            state.busy = 1
            state.capture_old = 1
            state.remaining = 2
        elif capture_old:
            # Capture is a separate phase from publishing the response, so a
            # full response Queue cannot lose the RAM's read-first value.
            state.request.data = memory.result()
            state.capture_old = 0
            state.remaining = remaining - 1
        elif remaining > 1:
            state.remaining = remaining - 1
        elif eligible:
            responses.push(response)
            state.busy = 0

    step()
    return BankResult(
        input_ready=requests.ready,
        output_valid=responses.valid,
        response=responses.data,
        accepted=accepted,
        enqueued=eligible & responses.ready,
    )


@ac.module
def MemoryBanks(valid: ac.u1, request: BankRequest, take: ac.u1) -> BanksResult:
    inputs = ac.queue[BankRequest](
        in_valid=valid, in_data=request, depth=8, ready_policy="local_occupancy"
    )
    merged = ac.queue[BankRequest](
        out_ready=take, depth=2, ready_policy="local_occupancy"
    )
    bank0 = Bank()
    bank1 = Bank()
    bank2 = Bank()
    bank3 = Bank()
    input_pending = inputs.valid
    input_head = inputs.data
    merge_space = merged.ready

    @ac.rule
    def route_request() -> None:
        head = inputs.pop()
        if head.bank == 0:
            bank0.enqueue(head)
        elif head.bank == 1:
            bank1.enqueue(head)
        elif head.bank == 2:
            bank2.enqueue(head)
        else:
            bank3.enqueue(head)

    @ac.rule
    def merge_response() -> None:
        if bank0.output_valid:
            response = bank0.dequeue()
        elif bank1.output_valid:
            response = bank1.dequeue()
        elif bank2.output_valid:
            response = bank2.dequeue()
        else:
            response = bank3.dequeue()
        merged.push(response)

    route_request()
    merge_response()
    route0 = input_pending & (input_head.bank == 0) & bank0.input_ready
    route1 = input_pending & (input_head.bank == 1) & bank1.input_ready
    route2 = input_pending & (input_head.bank == 2) & bank2.input_ready
    route3 = input_pending & (input_head.bank == 3) & bank3.input_ready
    merged0 = merge_space & bank0.output_valid
    merged1 = merge_space & ~bank0.output_valid & bank1.output_valid
    merged2 = (
        merge_space & ~bank0.output_valid & ~bank1.output_valid & bank2.output_valid
    )
    merged3 = (
        merge_space
        & ~bank0.output_valid
        & ~bank1.output_valid
        & ~bank2.output_valid
        & bank3.output_valid
    )
    return BanksResult(
        input_ready=inputs.ready,
        output_valid=merged.valid,
        response=merged.data,
        route=ac.concat(route3, route2, route1, route0),
        accepted=ac.concat(
            bank3.accepted, bank2.accepted, bank1.accepted, bank0.accepted
        ),
        enqueued=ac.concat(
            bank3.enqueued, bank2.enqueued, bank1.enqueued, bank0.enqueued
        ),
        response_valid=ac.concat(
            bank3.output_valid,
            bank2.output_valid,
            bank1.output_valid,
            bank0.output_valid,
        ),
        merged=ac.concat(merged3, merged2, merged1, merged0),
    )
