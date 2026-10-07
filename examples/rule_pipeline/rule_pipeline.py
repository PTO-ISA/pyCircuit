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


@ac.rule
def increment(token: RuleToken) -> RuleToken:
    result = token
    result.value = token.value + 1
    return result


@ac.module
def RulePipeline(  # noqa: N802
    valid: ac.u1, data: RuleToken, take: ac.u1
) -> RuleResult:
    ready, available, token = ac.queue[RuleToken](
        valid, data, stage_ready, depth=2, ready_policy="downstream_pop"  # noqa: F821
    )
    incremented = increment(token)
    stage_ready, out_valid, out_data = ac.queue[RuleToken](
        available, incremented, take, depth=1, ready_policy="downstream_pop"
    )
    return RuleResult(ready=ready, valid=out_valid, data=out_data)
