"""A depth-two input buffer feeding a one-token increment result stage."""

import pycircuit as ac


@ac.struct
class RuleToken:
    value: ac.u16


@ac.struct
class RuleResult:
    ready: ac.u1
    valid: ac.u1
    data: RuleToken


def increment(token: RuleToken) -> RuleToken:
    result = token
    result.value = token.value + 1
    return result


@ac.module
def RulePipeline(  # noqa: N802
    valid: ac.u1, data: RuleToken, take: ac.u1
) -> RuleResult:
    ingress = ac.queue[RuleToken](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[RuleToken](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        token = ingress.pop()
        incremented = increment(token)
        egress.push(incremented)

    transfer()
    return RuleResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
