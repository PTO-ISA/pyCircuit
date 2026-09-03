from __future__ import annotations

import agentic_circuit as ac


@ac.config
class CoreConfig:
    engines: int
    schedule_entries: int
    rob_entries: int


@ac.struct
class PTOInst:
    sequence: ac.u8
    waits_for: ac.u8
    engine: ac.u2
    cycles: ac.u16
    value: ac.u32


@ac.system
def davincioo(cfg: ac.const[CoreConfig]) -> None:
    incoming = ac.source(PTOInst, depth=4, latency=1)

    with ac.scope("decode"):
        decoded = ac.compute(
            incoming,
            lambda item: item.with_fields(value=item.value + 1),
            depth=4,
            latency=1,
        )

    with ac.scope("pipeline"):
        pipelined = ac.pipeline(decoded, stages=2, depth=4)

    with ac.scope("dispatch"):
        scalar, vector, cube, tma = ac.route(
            pipelined,
            by=PTOInst.engine,
            outputs=cfg.engines,
            depth=8,
            latency=1,
        )
        dispatched = ac.merge(
            scalar,
            vector,
            cube,
            tma,
            policy=ac.round_robin,
            depth=8,
            latency=1,
        )

    with ac.scope("schedule"):
        completed = ac.schedule(
            dispatched,
            by=PTOInst.sequence,
            waits_for=PTOInst.waits_for,
            resource=PTOInst.engine,
            cost=PTOInst.cycles,
            entries=cfg.schedule_entries,
            resources=cfg.engines,
            no_dependency=255,
            depth=16,
            latency=1,
        )

    with ac.scope("retire"):
        retired = ac.reorder(
            completed,
            by=PTOInst.sequence,
            entries=cfg.rob_entries,
            start=0,
            depth=8,
            latency=1,
        )

    ac.observe(retired)
    ac.sink(retired)


specialization = ac.jit(
    davincioo,
    cfg=CoreConfig(
        engines=4,
        schedule_entries=16,
        rob_entries=64,
    ),
)
