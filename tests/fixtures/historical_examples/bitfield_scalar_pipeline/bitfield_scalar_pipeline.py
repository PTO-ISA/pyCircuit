"""Permute a word and restore its original mode bits without losing X/Z."""

import pycircuit as ac


@ac.struct
class ScalarResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u32


@ac.module
def BitfieldScalarPipeline(  # noqa: N802
    valid: ac.u1, data: ac.u32, take: ac.u1
) -> ScalarResult:
    ingress = ac.queue[ac.u32](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[ac.u32](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        word = ingress.pop()
        permuted = ac.concat(word[:17], word[20:32], word[:3])
        egress.push(permuted)

    transfer()
    return ScalarResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
