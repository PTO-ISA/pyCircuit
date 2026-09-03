from __future__ import annotations

from agentic_circuit import ResourceRef, process, system


class RuntimeBool:
    pass


class Transaction:
    pass


class RequestQueue:
    pass


class Memory:
    pass


class Consumer:
    pass


class Target:
    pass


@system
def main() -> None:
    return None


@process
def controller(
    ready: RuntimeBool,
    requests: ResourceRef[RequestQueue, Consumer],
    memory: ResourceRef[Memory, Target],
) -> None:
    if ready:
        message = try_recv(requests)
    else:
        message = schedule(requests)
    wait_for(memory)
    record_stat(message)


@process
def bounded_counter(
    requests: ResourceRef[RequestQueue, Consumer],
) -> None:
    for index in range(3):
        record_stat(index)
    wait_for(requests)


@process
def terminating_process(ready: RuntimeBool) -> None:
    if ready:
        return
    else:
        return
