"""Dependent concrete types projected from one nested JIT configuration."""

from __future__ import annotations

import agentic_circuit as ac


@ac.config
class CacheConfig:
    sets: int
    ways: int
    line_bytes: int


@ac.config
class CoreConfig:
    cache: CacheConfig
    lanes: int


CFG = ac.param[CoreConfig]("cfg")


@ac.struct
class Lookup:
    set_index: ac.bits[ac.index_width(CFG.cache.sets)]
    candidates: ac.array[CFG.cache.ways, ac.u8]
    lane_count: ac.bits[ac.count_width(CFG.lanes)]


@ac.system
def nested_config_types(value: Lookup, *, cfg: ac.const[CoreConfig]) -> Lookup:
    ac.static_assert(cfg.cache.sets > 0, "cache sets must be positive")
    ac.static_assert(cfg.cache.ways > 0, "cache ways must be positive")
    ac.static_assert(
        cfg.cache.line_bytes & (cfg.cache.line_bytes - 1) == 0,
        "cache line bytes must be a power of two",
    )
    ac.static_assert(
        cfg.cache.sets % cfg.lanes == 0,
        "cache sets must be divisible by lanes",
    )
    return value


specialization = ac.jit(
    nested_config_types,
    cfg=CoreConfig(
        cache=CacheConfig(sets=64, ways=4, line_bytes=64),
        lanes=4,
    ),
)
