from __future__ import annotations

from agentic_circuit import ResourceRef, process


class RuntimeBool:
    pass


class RequestQueue:
    pass


class Consumer:
    pass


class TraceCursor:
    pass


class CursorOwner:
    pass


@process
async def coroutine_process(requests: ResourceRef[RequestQueue, Consumer]) -> None:
    await receive(requests)


@process
def generator_process(requests: ResourceRef[RequestQueue, Consumer]) -> None:
    yield requests


@process
def busy_process(
    ready: RuntimeBool, requests: ResourceRef[RequestQueue, Consumer]
) -> None:
    while ready:
        try_recv(requests)


@process
def partial_busy_process(
    ready: RuntimeBool, requests: ResourceRef[RequestQueue, Consumer]
) -> None:
    while ready:
        if ready:
            wait_for(requests)


@process
def forked_cursor_process(
    cursor: ResourceRef[TraceCursor, CursorOwner],
) -> None:
    first = trace_next(cursor)
    second = trace_next(cursor)


@process
def undeclared_effect_process(
    requests: ResourceRef[RequestQueue, Consumer],
) -> None:
    mutate_topology(requests)
