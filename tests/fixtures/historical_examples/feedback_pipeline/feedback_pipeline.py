"""One feedback step per edge, with internal-token priority."""

# ruff: noqa: N802 -- hardware module names.
import pycircuit as ac


@ac.struct
class Item:
    value: ac.u32
    remaining: ac.u4


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Item


@ac.module
def FeedbackPipeline(valid: ac.u1, data: Item, take: ac.u1) -> Result:
    ingress = ac.queue[Item](depth=2, latency=1, in_valid=valid, in_data=data)
    feedback = ac.queue[Item](depth=1, latency=1)
    egress = ac.queue[Item](depth=1, latency=1, out_ready=take)

    @ac.rule
    def step() -> None:
        # Occupied feedback has priority over new input; no empty bypass.
        if feedback.valid:
            item = feedback.pop()
        else:
            item = ingress.pop()
        if item.remaining > 0:
            updated = item
            updated.value = item.value + 1
            updated.remaining = item.remaining - 1
            feedback.push(updated)
        else:
            egress.push(item)

    step()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
