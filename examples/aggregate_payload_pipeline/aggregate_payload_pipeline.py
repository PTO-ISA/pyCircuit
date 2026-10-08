"""Rotate a fixed lane Table and advance a pair through two queue stages."""

from pycircuit import bits, module, queue, rule, struct, table


@struct
class Pair:
    first: bits[3]
    second: bits[5]


@struct
class AggregatePacket:
    pair: Pair
    lanes: table[4, bits[4]]
    selected: bits[4]


@struct
class PipelineResult:
    ready: bits[1]
    valid: bits[1]
    data: AggregatePacket


@rule
def rotate(item: AggregatePacket) -> AggregatePacket:
    return AggregatePacket(
        pair=Pair(first=item.pair.first + 1, second=item.pair.second + 1),
        lanes=(item.lanes[1], item.lanes[2], item.lanes[3], item.lanes[0]),
        selected=item.lanes[2],
    )


@module
def AggregatePayloadPipeline(  # noqa: N802
    valid: bits[1], data: AggregatePacket, take: bits[1]
) -> PipelineResult:
    ready, available, item = queue[AggregatePacket](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    advanced = rotate(item)
    stage_ready, out_valid, out_data = queue[AggregatePacket](
        available, advanced, take, depth=1, ready_policy="downstream_pop"
    )
    return PipelineResult(ready=ready, valid=out_valid, data=out_data)
