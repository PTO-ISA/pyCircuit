"""Select one complete data token using an independently queued control."""

# ruff: noqa: F821, N802 -- forward queue wires and hardware root name.
import pycircuit as ac


@ac.struct
class SelectControl:
    route: ac.u1


@ac.struct
class SelectResult:
    control_ready: ac.u1
    lane0_ready: ac.u1
    lane1_ready: ac.u1
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
) -> SelectResult:
    control_ready, control_available, control_head = ac.queue[SelectControl](
        control_valid,
        control,
        result_ready
        and (lane0_available if control_head.route == 0 else lane1_available),
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    lane0_ready, lane0_available, lane0_head = ac.queue[ac.u64](
        lane0_valid,
        lane0_data,
        result_ready and control_available and control_head.route == 0,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    lane1_ready, lane1_available, lane1_head = ac.queue[ac.u64](
        lane1_valid,
        lane1_data,
        result_ready and control_available and control_head.route == 1,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    choose_zero = control_head.route == 0
    selected_available = lane0_available if choose_zero else lane1_available
    selected_data = lane0_head if choose_zero else lane1_head
    result_ready, out_valid, out_data = ac.queue[ac.u64](
        control_available and selected_available,
        selected_data,
        take,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    return SelectResult(
        control_ready=control_ready,
        lane0_ready=lane0_ready,
        lane1_ready=lane1_ready,
        valid=out_valid,
        data=out_data,
    )
