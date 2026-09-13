"""Two module instances specialized from distinct nested config values."""

from __future__ import annotations

import agentic_circuit as ac


@ac.config
class StageConfig:
    entries: int


@ac.config
class PipelineConfig:
    stage: StageConfig


CFG = ac.param[StageConfig]("cfg")


@ac.struct
class Entry:
    index: ac.bits[ac.index_width(CFG.entries)]


@ac.struct
class Packet:
    value: ac.u8


@ac.rule
def keep(entries, value: Packet) -> Packet:
    entries[0] = Entry(index=0)
    return value


@ac.module
def stage(value: Packet, *, cfg: ac.const[StageConfig]) -> Packet:
    entries: list[Entry] = [0] * cfg.entries
    result = keep(entries, value)
    return result


@ac.system
def multi_config_specialization(
    value: Packet,
    *,
    first: ac.const[StageConfig],
    second: ac.const[PipelineConfig],
) -> Packet:
    first_result = stage(value, cfg=first)
    second_result = stage(first_result, cfg=second.stage)
    return second_result


specialization = ac.jit(
    multi_config_specialization,
    first=StageConfig(entries=5),
    second=PipelineConfig(stage=StageConfig(entries=6)),
)
