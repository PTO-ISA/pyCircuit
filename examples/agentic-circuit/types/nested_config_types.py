"""Dependent concrete types projected from one nested JIT configuration."""

from __future__ import annotations

import agentic_circuit as ac


@ac.config
class CacheConfig:
    sets: ac.static_int(width=64, signed=False)
    ways: ac.static_int(width=64, signed=False)
    line_bytes: ac.static_int(width=64, signed=False)


@ac.config
class CoreConfig:
    cache: CacheConfig
    lanes: ac.static_int(width=64, signed=False)


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
