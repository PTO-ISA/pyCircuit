"""Select one complete data token using an independently queued control."""

# ruff: noqa: N802 -- hardware root name.
import pycircuit as ac


@ac.struct
class SelectControl:
    route: ac.u1


@ac.struct
class Channel:
    valid: ac.u1
    data: ac.u64


@ac.module
def SelectPipeline(
    control_valid: ac.u1,
    control: SelectControl,
    lane0_valid: ac.u1,
    lane0_data: ac.u64,
    lane1_valid: ac.u1,
    lane1_data: ac.u64,
    take: ac.u1,
) -> tuple[ac.u1, ac.u1, ac.u1, Channel]:
    commands = ac.queue[SelectControl](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=control_valid,
        in_data=control,
    )
    lane0 = ac.queue[ac.u64](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=lane0_valid,
        in_data=lane0_data,
    )
    lane1 = ac.queue[ac.u64](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=lane1_valid,
        in_data=lane1_data,
    )
    result = ac.queue[ac.u64](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        out_ready=take,
    )

    @ac.rule
    def select() -> None:
        command = commands.pop()
        if command.route == 0:
            value = lane0.pop()
        else:
            value = lane1.pop()
        result.push(value)

    select()
    return (
        commands.ready,
        lane0.ready,
        lane1.ready,
        Channel(valid=result.valid, data=result.data),
    )
