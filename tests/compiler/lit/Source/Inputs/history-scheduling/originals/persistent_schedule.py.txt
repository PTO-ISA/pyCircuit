"""Bounded high-level schedule with persistent completion readiness."""

import agentic_circuit as ac


@ac.struct
class ScheduleToken:
    sequence: ac.u8
    waits_for: ac.u8
    resource: ac.u2
    cost: ac.u8
    value: ac.u16


@ac.system
def schedule_v2() -> None:
    incoming = ac.source(ScheduleToken, depth=4, latency=1)
    completed = ac.schedule(
        incoming,
        by=ScheduleToken.sequence,
        waits_for=ScheduleToken.waits_for,
        resource=ScheduleToken.resource,
        cost=ScheduleToken.cost,
        entries=4,
        resources=2,
        no_dependency=255,
        depth=4,
        latency=1,
    )
    ac.sink(completed)
