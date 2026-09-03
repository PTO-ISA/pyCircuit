import agentic_circuit as ac


@ac.struct
class WorkItem:
    sequence_id: ac.u8
    opcode: ac.u8
    route: ac.u2
    waits_for: ac.u8
    cycles: ac.u16
    value: ac.u64


@ac.system
def davincioo_queue_model() -> None:
    trace = ac.source(WorkItem, depth=16, latency=1)

    with ac.scope("frontend"):
        prepared = trace.apply(
            lambda item: item.with_fields(value=item.value + 1),
            depth=4,
            latency=1,
        )

    with ac.scope("dependency"):
        scheduled = prepared.depend(
            key=lambda item: item.sequence_id,
            waits_for=lambda item: item.waits_for,
            resource=lambda item: item.route,
            cost=lambda item: item.cycles,
            capacity=8,
            resources=4,
            no_dependency=255,
            depth=16,
            latency=1,
        )
        ac.observe(scheduled)

    with ac.scope("dispatch"):
        scalar, vector, cube, tma = scheduled.route(
            outputs=4,
            key=lambda item: item.route,
            depth=8,
            latency=1,
        )

    with ac.scope("scalar_engine"):
        scalar_done = scalar.apply(lambda item: item.with_fields(value=item.value + 1))
    with ac.scope("vector_engine"):
        vector_done = vector.apply(lambda item: item.with_fields(value=item.value + 2))
        ac.observe(vector_done)
    with ac.scope("cube_engine"):
        cube_done = cube.apply(lambda item: item.with_fields(value=item.value + 3))
    with ac.scope("tma_engine"):
        tma_done = tma.apply(lambda item: item.with_fields(value=item.value + 4))
        ac.observe(tma_done)

    completed = scalar_done.merge(
        vector_done,
        cube_done,
        tma_done,
        policy="round_robin",
        depth=8,
        latency=1,
    )
    ac.observe(completed)

    ordered = completed.reorder(
        key=lambda item: item.sequence_id,
        capacity=64,
        start=0,
        depth=8,
        latency=1,
    )

    with ac.scope("retire"):
        retired = ordered.apply(
            lambda item: item.with_fields(
                value=item.value + 100,
            )
        )

    ac.sink(retired)
