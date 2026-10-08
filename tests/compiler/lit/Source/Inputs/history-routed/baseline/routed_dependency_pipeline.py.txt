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
def routed_dependency_pipeline() -> None:
    incoming = ac.source(WorkItem, depth=16, latency=1)

    with ac.scope("frontend"):
        prepared = incoming.apply(
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
        route_0, route_1, route_2, route_3 = scheduled.route(
            outputs=4,
            key=lambda item: item.route,
            depth=8,
            latency=1,
        )

    with ac.scope("route_0"):
        route_0_done = route_0.apply(
            lambda item: item.with_fields(value=item.value + 1)
        )
    with ac.scope("route_1"):
        route_1_done = route_1.apply(
            lambda item: item.with_fields(value=item.value + 2)
        )
        ac.observe(route_1_done)
    with ac.scope("route_2"):
        route_2_done = route_2.apply(
            lambda item: item.with_fields(value=item.value + 3)
        )
    with ac.scope("route_3"):
        route_3_done = route_3.apply(
            lambda item: item.with_fields(value=item.value + 4)
        )
        ac.observe(route_3_done)

    completed = route_0_done.merge(
        route_1_done,
        route_2_done,
        route_3_done,
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

    with ac.scope("output"):
        output = ordered.apply(
            lambda item: item.with_fields(
                value=item.value + 100,
            )
        )

    ac.sink(output)
