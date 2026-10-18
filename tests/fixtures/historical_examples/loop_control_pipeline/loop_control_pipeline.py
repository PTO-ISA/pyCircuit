"""One feedback step per edge, with internal-token priority."""

# ruff: noqa: N802 -- hardware module names.
import pycircuit as ac


@ac.struct
class LoopToken:
    remaining: ac.u4
    stop: ac.u1
    skip: ac.u1


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: LoopToken


@ac.module
def LoopControlPipeline(valid: ac.u1, data: LoopToken, take: ac.u1) -> Result:
    ingress = ac.queue[LoopToken](depth=2, latency=1, in_valid=valid, in_data=data)
    feedback = ac.queue[LoopToken](depth=1, latency=1)
    egress = ac.queue[LoopToken](depth=1, latency=1, out_ready=take)

    @ac.rule
    def step() -> None:
        # Occupied feedback has priority over new input; no empty bypass.
        if feedback.valid:
            item = feedback.pop()
        else:
            item = ingress.pop()
        if item.remaining > 0 and not item.stop:
            updated = item
            updated.remaining = item.remaining - 1
            feedback.push(updated)
        else:
            egress.push(item)

    step()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
