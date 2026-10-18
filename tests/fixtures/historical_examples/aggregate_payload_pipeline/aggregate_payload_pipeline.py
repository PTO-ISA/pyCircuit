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
    ingress = queue[AggregatePacket](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = queue[AggregatePacket](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @rule
    def transfer() -> None:
        item = ingress.pop()
        advanced = rotate(item)
        egress.push(advanced)

    transfer()
    return PipelineResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
