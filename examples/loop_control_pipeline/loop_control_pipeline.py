"""One feedback step per edge, with internal-token priority."""

# ruff: noqa: F821, N802 -- forward module result fields and hardware names.
import pycircuit as ac


@ac.struct
class LoopToken:
    remaining: ac.u4
    stop: ac.u1
    skip: ac.u1


@ac.struct
class StepResult:
    source_take: ac.u1
    feedback_valid: ac.u1
    feedback_data: LoopToken
    feedback_take: ac.u1
    exit_valid: ac.u1
    exit_data: LoopToken


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: LoopToken


@ac.module
def Step(
    source_valid: ac.u1,
    source_data: LoopToken,
    feedback_valid: ac.u1,
    feedback_data: LoopToken,
    output_ready: ac.u1,
) -> StepResult:
    chosen = feedback_data if feedback_valid else source_data
    available = feedback_valid or source_valid
    continuing = (chosen.remaining > 0) and not chosen.stop
    can_proceed = continuing or output_ready
    updated = chosen
    updated.remaining = chosen.remaining - 1
    return StepResult(
        source_take=(not feedback_valid) and can_proceed,
        feedback_valid=available and continuing,
        feedback_data=updated,
        feedback_take=can_proceed,
        exit_valid=available and not continuing,
        exit_data=chosen,
    )


@ac.module
def LoopControlPipeline(valid: ac.u1, data: LoopToken, take: ac.u1) -> Result:
    ready, available, incoming = ac.queue[LoopToken](
        valid,
        data,
        step.source_take,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    _feedback_ready, feedback_valid, feedback_data = ac.queue[LoopToken](
        step.feedback_valid,
        step.feedback_data,
        step.feedback_take,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    output_ready, out_valid, out_data = ac.queue[LoopToken](
        step.exit_valid,
        step.exit_data,
        take,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    step = Step(available, incoming, feedback_valid, feedback_data, output_ready)
    return Result(ready=ready, valid=out_valid, data=out_data)
