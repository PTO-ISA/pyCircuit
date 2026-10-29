import pycircuit as ac
from allocation_repro.contracts import POOL_CAPACITY, ALLOCATE, RELEASE, LOOKUP, Entry, Request

@ac.module
def AllocationPool() -> None:
    entries = ac.table[POOL_CAPACITY, Entry](init=0)
    completed = ac.queue[Request](depth=1, latency=1, ready_policy='downstream_pop')
    allocate_ack = ac.queue[Request](depth=1, latency=1, ready_policy='downstream_pop')
    lookup_ack = ac.queue[Request](depth=1, latency=1, ready_policy='downstream_pop')
    release_ack = ac.queue[Request](depth=1, latency=1, ready_policy='downstream_pop')
    refused = ac.queue[Request](depth=1, latency=1, ready_policy='downstream_pop')

    @ac.method
    def submit(request: Request) -> None:
        existing_index, existing_valid = entries.argmin(
            where=lambda entry: entry.live and entry.group_id == request.group_id and (entry.entry_id == request.entry_id),
            key=lambda entry: entry.position,
        )
        free_index, free_valid = entries.argmin(
            where=lambda entry: not entry.live,
            key=lambda entry: entry.position,
        )
        existing = entries[existing_index]
        matched = existing_valid != 0
        has_free = free_valid != 0
        is_allocate = request.operation == ALLOCATE
        is_lookup = request.operation == LOOKUP
        is_release = request.operation == RELEASE
        reachable = request.current_position >= existing.position
        allocates = is_allocate and (not matched) and has_free
        lookups = is_lookup and matched and reachable
        releases = is_release and matched
        if allocates:
            entries[free_index] = Entry(
                group_id=request.group_id,
                entry_id=request.entry_id,
                position=request.current_position,
                live=True,
            )
        if releases:
            entries[existing_index].live = False
        target = request.current_position
        distance: ac.u6 = 0
        if lookups:
            target = existing.position
            distance = request.current_position - existing.position
        response = request
        response.accepted = allocates or lookups or releases
        response.matched = matched
        response.exhausted = is_allocate and (not matched) and (not has_free)
        response.stale = is_lookup and matched and (not reachable)
        response.target_position = target
        response.steps = distance
        completed.push(response)

    @ac.rule
    def route() -> None:
        response = completed.pop()
        if response.operation == ALLOCATE:
            allocate_ack.push(response)
        elif response.operation == LOOKUP:
            lookup_ack.push(response)
        elif response.operation == RELEASE:
            release_ack.push(response)
        else:
            refused.push(response)

    @ac.method
    def allocate_response() -> Request:
        return allocate_ack.pop()

    @ac.method
    def lookup_response() -> Request:
        return lookup_ack.pop()

    @ac.method
    def release_response() -> Request:
        return release_ack.pop()

    @ac.method
    def refused_response() -> Request:
        return refused.pop()
