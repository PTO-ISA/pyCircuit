from __future__ import annotations

import agentic_circuit as ac


@ac.config
class Config:
    rate: int


@ac.system
def multirate_compute(cfg: ac.const[Config]) -> None:
    incoming = ac.source(int, depth=8, rate=cfg.rate)
    computed = ac.compute(
        incoming,
        lambda item: item + 1,
        depth=8,
        rate=cfg.rate,
    )
    pipelined = ac.pipeline(
        computed,
        stages=2,
        depth=8,
        rate=cfg.rate,
    )
    ac.sink(pipelined)


specialization = ac.jit(multirate_compute, cfg=Config(rate=4))
