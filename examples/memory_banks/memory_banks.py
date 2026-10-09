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


@ac.rule
def Advance(state, accepted, request, raw_data, release, initial_delay):
    # Old busy excludes acceptance on the response-release edge.
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
def Bank(valid: ac.u1, request: BankRequest, take: ac.u1) -> BankResult:
    state: BankState = BankState()
    request_ready = ~state.busy
    input_ready, pending, head = ac.queue[BankRequest](
        valid, request, request_ready, depth=2
    )
    accepted = pending & request_ready
    raw = ac.sync_mem[ac.u16](
        accepted,
        head.offset,
        accepted & head.write,
        head.offset,
        head.data,
        3,
        depth=16,
    )
    eligible = state.busy & (state.remaining == 1)
    old_data = raw if state.capture_old else state.request.data
    response = state.request
    response.data = old_data
    response_space, response_valid, response_head = ac.queue[BankRequest](
        eligible, response, take, depth=2
    )
    release = eligible & response_space
    Advance(state, accepted, head, raw, release, 2)
    return BankResult(
        input_ready=input_ready,
        output_valid=response_valid,
        response=response_head,
        accepted=accepted,
        enqueued=release,
    )


@ac.module
def MemoryBanks(valid: ac.u1, request: BankRequest, take: ac.u1) -> BanksResult:
    input_ready, input_pending, input_head = ac.queue[BankRequest](
        valid,
        request,
        input_pending
        & (
            bank0.input_ready
            if input_head.bank == 0
            else (
                bank1.input_ready
                if input_head.bank == 1
                else bank2.input_ready if input_head.bank == 2 else bank3.input_ready
            )
        ),
        depth=8,
    )
    bank0 = Bank(input_pending & (input_head.bank == 0), input_head, merge_space)
    bank1 = Bank(
        input_pending & (input_head.bank == 1),
        input_head,
        merge_space & ~bank0.output_valid,
    )
    bank2 = Bank(
        input_pending & (input_head.bank == 2),
        input_head,
        merge_space & ~bank0.output_valid & ~bank1.output_valid,
    )
    bank3 = Bank(
        input_pending & (input_head.bank == 3),
        input_head,
        merge_space & ~bank0.output_valid & ~bank1.output_valid & ~bank2.output_valid,
    )
    selected = (
        bank0.response
        if bank0.output_valid
        else (
            bank1.response
            if bank1.output_valid
            else bank2.response if bank2.output_valid else bank3.response
        )
    )
    available = (
        bank0.output_valid
        | bank1.output_valid
        | bank2.output_valid
        | bank3.output_valid
    )
    merge_space, output_valid, response = ac.queue[BankRequest](
        available, selected, take, depth=2
    )
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
        input_ready=input_ready,
        output_valid=output_valid,
        response=response,
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
