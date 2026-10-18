"""Preserve a 13-bit payload while replacing both endpoint-zero counts."""

import pycircuit as ac


@ac.struct
class Item:
    value: ac.u13
    leading: ac.u4
    trailing: ac.u4


@ac.struct
class CountZerosResult:
    ready: ac.u1
    valid: ac.u1
    data: Item


@ac.module
def CountZerosPipeline(  # noqa: N802
    valid: ac.u1, data: Item, take: ac.u1
) -> CountZerosResult:
    ingress = ac.queue[Item](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[Item](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take,
    )

    @ac.rule
    def transfer() -> None:
        head = ingress.pop()
        counted = Item(
            value=head.value,
            leading=ac.count_leading_zeros(head.value),
            trailing=ac.count_trailing_zeros(head.value),
        )
        egress.push(counted)

    transfer()
    return CountZerosResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
