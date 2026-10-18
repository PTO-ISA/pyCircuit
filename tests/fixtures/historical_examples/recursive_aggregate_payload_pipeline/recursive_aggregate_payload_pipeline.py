"""Transform nested nominal records and an Enum Table without changing the input."""

from enum import Enum

from pycircuit import bits, encoding, module, queue, rule, struct, table


@encoding(width=1)
class Mode(Enum):
    IDLE = 0
    RUN = 1


@struct
class Header:
    code: bits[3]


@struct
class Tagged:
    mode: Mode
    value: bits[3]


@struct
class Nested:
    header: Header
    value: bits[3]


@struct
class Packet:
    tagged: Tagged
    nested: Nested
    modes: table[2, Mode]
    flag: bits[1]


@struct
class PipelineResult:
    ready: bits[1]
    valid: bits[1]
    data: Packet


def update(item: Packet) -> Packet:
    return Packet(
        tagged=Tagged(mode=Mode.RUN, value=item.tagged.value),
        nested=Nested(
            header=Header(code=item.nested.header.code + 1),
            value=item.nested.value,
        ),
        modes=(item.modes[1], item.modes[0]),
        flag=item.modes[0] == Mode.IDLE,
    )


@module
def RecursiveAggregatePayloadPipeline(  # noqa: N802
    valid: bits[1], data: Packet, take: bits[1]
) -> PipelineResult:
    ingress = queue[Packet](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = queue[Packet](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @rule
    def transfer() -> None:
        item = ingress.pop()
        advanced = update(item)
        egress.push(advanced)

    transfer()
    return PipelineResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
