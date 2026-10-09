"""Closed full-graph systems with independently derived literal phase inputs."""

from history_routed.records import WorkItem
from history_routed.routed_dependency_pipeline import pipeline
from pycircuit import log, rule, struct, system, u1, u16


@struct
class Stimulus:
    valid: u1
    take: u1
    data: WorkItem


@struct
class ExpectedView:
    available: u1
    head: WorkItem


@struct
class Ready:
    value: u1


@rule
def observe(
    epoch,
    result,
    expected_ready,
    expected_output,
    expected_scheduled,
    expected_route_1_done,
    expected_route_3_done,
    expected_completed,
):
    assert result.ready == expected_ready.value, "incoming capacity"
    log("info", "epoch", epoch)
    log("info", "ready", result.ready)
    assert result.output_available == expected_output.available, "output availability"
    log("info", "output_available", result.output_available)
    if result.output_available:
        assert (
            result.output_head.sequence_id == expected_output.head.sequence_id
        ), "output sequence_id"
        assert result.output_head.opcode == expected_output.head.opcode, "output opcode"
        assert result.output_head.route == expected_output.head.route, "output route"
        assert (
            result.output_head.waits_for == expected_output.head.waits_for
        ), "output waits_for"
        assert result.output_head.cycles == expected_output.head.cycles, "output cycles"
        assert result.output_head.value == expected_output.head.value, "output value"
    log(
        "info",
        "output_sequence_id",
        result.output_head.sequence_id if result.output_available else 0,
    )
    log(
        "info",
        "output_opcode",
        result.output_head.opcode if result.output_available else 0,
    )
    log(
        "info",
        "output_route",
        result.output_head.route if result.output_available else 0,
    )
    log(
        "info",
        "output_waits_for",
        result.output_head.waits_for if result.output_available else 0,
    )
    log(
        "info",
        "output_cycles",
        result.output_head.cycles if result.output_available else 0,
    )
    log(
        "info",
        "output_value",
        result.output_head.value if result.output_available else 0,
    )
    assert (
        result.scheduled_available == expected_scheduled.available
    ), "scheduled availability"
    log("info", "scheduled_available", result.scheduled_available)
    if result.scheduled_available:
        assert (
            result.scheduled_head.sequence_id == expected_scheduled.head.sequence_id
        ), "scheduled sequence_id"
        assert (
            result.scheduled_head.opcode == expected_scheduled.head.opcode
        ), "scheduled opcode"
        assert (
            result.scheduled_head.route == expected_scheduled.head.route
        ), "scheduled route"
        assert (
            result.scheduled_head.waits_for == expected_scheduled.head.waits_for
        ), "scheduled waits_for"
        assert (
            result.scheduled_head.cycles == expected_scheduled.head.cycles
        ), "scheduled cycles"
        assert (
            result.scheduled_head.value == expected_scheduled.head.value
        ), "scheduled value"
    log(
        "info",
        "scheduled_sequence_id",
        result.scheduled_head.sequence_id if result.scheduled_available else 0,
    )
    log(
        "info",
        "scheduled_opcode",
        result.scheduled_head.opcode if result.scheduled_available else 0,
    )
    log(
        "info",
        "scheduled_route",
        result.scheduled_head.route if result.scheduled_available else 0,
    )
    log(
        "info",
        "scheduled_waits_for",
        result.scheduled_head.waits_for if result.scheduled_available else 0,
    )
    log(
        "info",
        "scheduled_cycles",
        result.scheduled_head.cycles if result.scheduled_available else 0,
    )
    log(
        "info",
        "scheduled_value",
        result.scheduled_head.value if result.scheduled_available else 0,
    )
    assert (
        result.route_1_done_available == expected_route_1_done.available
    ), "route_1_done availability"
    log("info", "route_1_done_available", result.route_1_done_available)
    if result.route_1_done_available:
        assert (
            result.route_1_done_head.sequence_id
            == expected_route_1_done.head.sequence_id
        ), "route_1_done sequence_id"
        assert (
            result.route_1_done_head.opcode == expected_route_1_done.head.opcode
        ), "route_1_done opcode"
        assert (
            result.route_1_done_head.route == expected_route_1_done.head.route
        ), "route_1_done route"
        assert (
            result.route_1_done_head.waits_for == expected_route_1_done.head.waits_for
        ), "route_1_done waits_for"
        assert (
            result.route_1_done_head.cycles == expected_route_1_done.head.cycles
        ), "route_1_done cycles"
        assert (
            result.route_1_done_head.value == expected_route_1_done.head.value
        ), "route_1_done value"
    log(
        "info",
        "route_1_done_sequence_id",
        result.route_1_done_head.sequence_id if result.route_1_done_available else 0,
    )
    log(
        "info",
        "route_1_done_opcode",
        result.route_1_done_head.opcode if result.route_1_done_available else 0,
    )
    log(
        "info",
        "route_1_done_route",
        result.route_1_done_head.route if result.route_1_done_available else 0,
    )
    log(
        "info",
        "route_1_done_waits_for",
        result.route_1_done_head.waits_for if result.route_1_done_available else 0,
    )
    log(
        "info",
        "route_1_done_cycles",
        result.route_1_done_head.cycles if result.route_1_done_available else 0,
    )
    log(
        "info",
        "route_1_done_value",
        result.route_1_done_head.value if result.route_1_done_available else 0,
    )
    assert (
        result.route_3_done_available == expected_route_3_done.available
    ), "route_3_done availability"
    log("info", "route_3_done_available", result.route_3_done_available)
    if result.route_3_done_available:
        assert (
            result.route_3_done_head.sequence_id
            == expected_route_3_done.head.sequence_id
        ), "route_3_done sequence_id"
        assert (
            result.route_3_done_head.opcode == expected_route_3_done.head.opcode
        ), "route_3_done opcode"
        assert (
            result.route_3_done_head.route == expected_route_3_done.head.route
        ), "route_3_done route"
        assert (
            result.route_3_done_head.waits_for == expected_route_3_done.head.waits_for
        ), "route_3_done waits_for"
        assert (
            result.route_3_done_head.cycles == expected_route_3_done.head.cycles
        ), "route_3_done cycles"
        assert (
            result.route_3_done_head.value == expected_route_3_done.head.value
        ), "route_3_done value"
    log(
        "info",
        "route_3_done_sequence_id",
        result.route_3_done_head.sequence_id if result.route_3_done_available else 0,
    )
    log(
        "info",
        "route_3_done_opcode",
        result.route_3_done_head.opcode if result.route_3_done_available else 0,
    )
    log(
        "info",
        "route_3_done_route",
        result.route_3_done_head.route if result.route_3_done_available else 0,
    )
    log(
        "info",
        "route_3_done_waits_for",
        result.route_3_done_head.waits_for if result.route_3_done_available else 0,
    )
    log(
        "info",
        "route_3_done_cycles",
        result.route_3_done_head.cycles if result.route_3_done_available else 0,
    )
    log(
        "info",
        "route_3_done_value",
        result.route_3_done_head.value if result.route_3_done_available else 0,
    )
    assert (
        result.completed_available == expected_completed.available
    ), "completed availability"
    log("info", "completed_available", result.completed_available)
    if result.completed_available:
        assert (
            result.completed_head.sequence_id == expected_completed.head.sequence_id
        ), "completed sequence_id"
        assert (
            result.completed_head.opcode == expected_completed.head.opcode
        ), "completed opcode"
        assert (
            result.completed_head.route == expected_completed.head.route
        ), "completed route"
        assert (
            result.completed_head.waits_for == expected_completed.head.waits_for
        ), "completed waits_for"
        assert (
            result.completed_head.cycles == expected_completed.head.cycles
        ), "completed cycles"
        assert (
            result.completed_head.value == expected_completed.head.value
        ), "completed value"
    log(
        "info",
        "completed_sequence_id",
        result.completed_head.sequence_id if result.completed_available else 0,
    )
    log(
        "info",
        "completed_opcode",
        result.completed_head.opcode if result.completed_available else 0,
    )
    log(
        "info",
        "completed_route",
        result.completed_head.route if result.completed_available else 0,
    )
    log(
        "info",
        "completed_waits_for",
        result.completed_head.waits_for if result.completed_available else 0,
    )
    log(
        "info",
        "completed_cycles",
        result.completed_head.cycles if result.completed_available else 0,
    )
    log(
        "info",
        "completed_value",
        result.completed_head.value if result.completed_available else 0,
    )
    epoch = epoch + 1


@rule
def four_routes_live_dependencies_stimulus(epoch: u16) -> Stimulus:
    valid: u1 = 0
    take: u1 = 0
    data = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 0:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=0,
            opcode=7,
            route=0,
            waits_for=255,
            cycles=12,
            value=18446744073709551615,
        )
    if epoch == 1:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=1, opcode=36, route=1, waits_for=0, cycles=3, value=1001
        )
    if epoch == 2:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=2,
            opcode=65,
            route=2,
            waits_for=1,
            cycles=4,
            value=18446744073709551613,
        )
    if epoch == 3:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=3, opcode=94, route=3, waits_for=2, cycles=2, value=3003
        )
    if epoch >= 4 and epoch <= 39:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 40:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=4,
            opcode=123,
            route=0,
            waits_for=255,
            cycles=3,
            value=18446744073709551611,
        )
    if epoch == 41:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=5, opcode=152, route=1, waits_for=255, cycles=4, value=5005
        )
    if epoch == 42:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=6,
            opcode=181,
            route=2,
            waits_for=255,
            cycles=2,
            value=18446744073709551609,
        )
    if epoch == 43:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=7, opcode=210, route=3, waits_for=255, cycles=3, value=7007
        )
    if epoch == 44:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=8,
            opcode=239,
            route=0,
            waits_for=255,
            cycles=4,
            value=18446744073709551607,
        )
    if epoch == 45:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=9, opcode=12, route=1, waits_for=255, cycles=2, value=9009
        )
    if epoch == 46:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=10,
            opcode=41,
            route=2,
            waits_for=255,
            cycles=3,
            value=18446744073709551605,
        )
    if epoch == 47:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=11, opcode=70, route=3, waits_for=255, cycles=4, value=11011
        )
    if epoch == 48:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=12,
            opcode=99,
            route=0,
            waits_for=255,
            cycles=2,
            value=18446744073709551603,
        )
    if epoch == 49:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=13, opcode=128, route=1, waits_for=255, cycles=3, value=13013
        )
    if epoch == 50:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=14,
            opcode=157,
            route=2,
            waits_for=255,
            cycles=4,
            value=18446744073709551601,
        )
    if epoch == 51:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=15, opcode=186, route=3, waits_for=255, cycles=2, value=15015
        )
    if epoch >= 52 and epoch <= 80:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    return Stimulus(valid=valid, take=take, data=data)


@rule
def four_routes_live_dependencies_ready(epoch: u16) -> Ready:
    ready: u1 = 0
    if epoch >= 0 and epoch <= 80:
        ready = 1
    return Ready(value=ready)


@rule
def four_routes_live_dependencies_output(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 23:
        available = 1
        head = WorkItem(
            sequence_id=0, opcode=7, route=0, waits_for=255, cycles=12, value=101
        )
    if epoch == 27:
        available = 1
        head = WorkItem(
            sequence_id=1, opcode=36, route=1, waits_for=0, cycles=3, value=1104
        )
    if epoch == 32:
        available = 1
        head = WorkItem(
            sequence_id=2, opcode=65, route=2, waits_for=1, cycles=4, value=101
        )
    if epoch == 35:
        available = 1
        head = WorkItem(
            sequence_id=3, opcode=94, route=3, waits_for=2, cycles=2, value=3108
        )
    if epoch == 54:
        available = 1
        head = WorkItem(
            sequence_id=4, opcode=123, route=0, waits_for=255, cycles=3, value=97
        )
    if epoch == 56:
        available = 1
        head = WorkItem(
            sequence_id=5, opcode=152, route=1, waits_for=255, cycles=4, value=5108
        )
    if epoch == 58:
        available = 1
        head = WorkItem(
            sequence_id=6, opcode=181, route=2, waits_for=255, cycles=2, value=97
        )
    if epoch == 60:
        available = 1
        head = WorkItem(
            sequence_id=7, opcode=210, route=3, waits_for=255, cycles=3, value=7112
        )
    if epoch == 62:
        available = 1
        head = WorkItem(
            sequence_id=8, opcode=239, route=0, waits_for=255, cycles=4, value=93
        )
    if epoch == 64:
        available = 1
        head = WorkItem(
            sequence_id=9, opcode=12, route=1, waits_for=255, cycles=2, value=9112
        )
    if epoch == 66:
        available = 1
        head = WorkItem(
            sequence_id=10, opcode=41, route=2, waits_for=255, cycles=3, value=93
        )
    if epoch == 68:
        available = 1
        head = WorkItem(
            sequence_id=11, opcode=70, route=3, waits_for=255, cycles=4, value=11116
        )
    if epoch == 70:
        available = 1
        head = WorkItem(
            sequence_id=12, opcode=99, route=0, waits_for=255, cycles=2, value=89
        )
    if epoch == 72:
        available = 1
        head = WorkItem(
            sequence_id=13, opcode=128, route=1, waits_for=255, cycles=3, value=13116
        )
    if epoch == 74:
        available = 1
        head = WorkItem(
            sequence_id=14, opcode=157, route=2, waits_for=255, cycles=4, value=89
        )
    if epoch == 76:
        available = 1
        head = WorkItem(
            sequence_id=15, opcode=186, route=3, waits_for=255, cycles=2, value=15120
        )
    return ExpectedView(available=available, head=head)


@rule
def four_routes_live_dependencies_scheduled(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 17:
        available = 1
        head = WorkItem(
            sequence_id=0, opcode=7, route=0, waits_for=255, cycles=12, value=0
        )
    if epoch == 21:
        available = 1
        head = WorkItem(
            sequence_id=1, opcode=36, route=1, waits_for=0, cycles=3, value=1002
        )
    if epoch == 26:
        available = 1
        head = WorkItem(
            sequence_id=2,
            opcode=65,
            route=2,
            waits_for=1,
            cycles=4,
            value=18446744073709551614,
        )
    if epoch == 29:
        available = 1
        head = WorkItem(
            sequence_id=3, opcode=94, route=3, waits_for=2, cycles=2, value=3004
        )
    if epoch == 48:
        available = 1
        head = WorkItem(
            sequence_id=4,
            opcode=123,
            route=0,
            waits_for=255,
            cycles=3,
            value=18446744073709551612,
        )
    if epoch == 49:
        available = 1
        head = WorkItem(
            sequence_id=6,
            opcode=181,
            route=2,
            waits_for=255,
            cycles=2,
            value=18446744073709551610,
        )
    if epoch == 50:
        available = 1
        head = WorkItem(
            sequence_id=5, opcode=152, route=1, waits_for=255, cycles=4, value=5006
        )
    if epoch == 51:
        available = 1
        head = WorkItem(
            sequence_id=7, opcode=210, route=3, waits_for=255, cycles=3, value=7008
        )
    if epoch == 52:
        available = 1
        head = WorkItem(
            sequence_id=9, opcode=12, route=1, waits_for=255, cycles=2, value=9010
        )
    if epoch == 53:
        available = 1
        head = WorkItem(
            sequence_id=8,
            opcode=239,
            route=0,
            waits_for=255,
            cycles=4,
            value=18446744073709551608,
        )
    if epoch == 54:
        available = 1
        head = WorkItem(
            sequence_id=10,
            opcode=41,
            route=2,
            waits_for=255,
            cycles=3,
            value=18446744073709551606,
        )
    if epoch == 55:
        available = 1
        head = WorkItem(
            sequence_id=12,
            opcode=99,
            route=0,
            waits_for=255,
            cycles=2,
            value=18446744073709551604,
        )
    if epoch == 56:
        available = 1
        head = WorkItem(
            sequence_id=11, opcode=70, route=3, waits_for=255, cycles=4, value=11012
        )
    if epoch == 57:
        available = 1
        head = WorkItem(
            sequence_id=13, opcode=128, route=1, waits_for=255, cycles=3, value=13014
        )
    if epoch == 58:
        available = 1
        head = WorkItem(
            sequence_id=15, opcode=186, route=3, waits_for=255, cycles=2, value=15016
        )
    if epoch == 59:
        available = 1
        head = WorkItem(
            sequence_id=14,
            opcode=157,
            route=2,
            waits_for=255,
            cycles=4,
            value=18446744073709551602,
        )
    return ExpectedView(available=available, head=head)


@rule
def four_routes_live_dependencies_route_1_done(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 23:
        available = 1
        head = WorkItem(
            sequence_id=1, opcode=36, route=1, waits_for=0, cycles=3, value=1004
        )
    if epoch == 52:
        available = 1
        head = WorkItem(
            sequence_id=5, opcode=152, route=1, waits_for=255, cycles=4, value=5008
        )
    if epoch == 54:
        available = 1
        head = WorkItem(
            sequence_id=9, opcode=12, route=1, waits_for=255, cycles=2, value=9012
        )
    if epoch == 59:
        available = 1
        head = WorkItem(
            sequence_id=13, opcode=128, route=1, waits_for=255, cycles=3, value=13016
        )
    return ExpectedView(available=available, head=head)


@rule
def four_routes_live_dependencies_route_3_done(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 31:
        available = 1
        head = WorkItem(
            sequence_id=3, opcode=94, route=3, waits_for=2, cycles=2, value=3008
        )
    if epoch == 53:
        available = 1
        head = WorkItem(
            sequence_id=7, opcode=210, route=3, waits_for=255, cycles=3, value=7012
        )
    if epoch == 58:
        available = 1
        head = WorkItem(
            sequence_id=11, opcode=70, route=3, waits_for=255, cycles=4, value=11016
        )
    if epoch == 60:
        available = 1
        head = WorkItem(
            sequence_id=15, opcode=186, route=3, waits_for=255, cycles=2, value=15020
        )
    return ExpectedView(available=available, head=head)


@rule
def four_routes_live_dependencies_completed(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 20:
        available = 1
        head = WorkItem(
            sequence_id=0, opcode=7, route=0, waits_for=255, cycles=12, value=1
        )
    if epoch == 24:
        available = 1
        head = WorkItem(
            sequence_id=1, opcode=36, route=1, waits_for=0, cycles=3, value=1004
        )
    if epoch == 29:
        available = 1
        head = WorkItem(
            sequence_id=2, opcode=65, route=2, waits_for=1, cycles=4, value=1
        )
    if epoch == 32:
        available = 1
        head = WorkItem(
            sequence_id=3, opcode=94, route=3, waits_for=2, cycles=2, value=3008
        )
    if epoch == 51:
        available = 1
        head = WorkItem(
            sequence_id=4,
            opcode=123,
            route=0,
            waits_for=255,
            cycles=3,
            value=18446744073709551613,
        )
    if epoch == 52:
        available = 1
        head = WorkItem(
            sequence_id=6,
            opcode=181,
            route=2,
            waits_for=255,
            cycles=2,
            value=18446744073709551613,
        )
    if epoch == 53:
        available = 1
        head = WorkItem(
            sequence_id=5, opcode=152, route=1, waits_for=255, cycles=4, value=5008
        )
    if epoch == 54:
        available = 1
        head = WorkItem(
            sequence_id=7, opcode=210, route=3, waits_for=255, cycles=3, value=7012
        )
    if epoch == 55:
        available = 1
        head = WorkItem(
            sequence_id=9, opcode=12, route=1, waits_for=255, cycles=2, value=9012
        )
    if epoch == 56:
        available = 1
        head = WorkItem(
            sequence_id=8,
            opcode=239,
            route=0,
            waits_for=255,
            cycles=4,
            value=18446744073709551609,
        )
    if epoch == 57:
        available = 1
        head = WorkItem(
            sequence_id=10,
            opcode=41,
            route=2,
            waits_for=255,
            cycles=3,
            value=18446744073709551609,
        )
    if epoch == 58:
        available = 1
        head = WorkItem(
            sequence_id=12,
            opcode=99,
            route=0,
            waits_for=255,
            cycles=2,
            value=18446744073709551605,
        )
    if epoch == 59:
        available = 1
        head = WorkItem(
            sequence_id=11, opcode=70, route=3, waits_for=255, cycles=4, value=11016
        )
    if epoch == 60:
        available = 1
        head = WorkItem(
            sequence_id=13, opcode=128, route=1, waits_for=255, cycles=3, value=13016
        )
    if epoch == 61:
        available = 1
        head = WorkItem(
            sequence_id=15, opcode=186, route=3, waits_for=255, cycles=2, value=15020
        )
    if epoch == 62:
        available = 1
        head = WorkItem(
            sequence_id=14,
            opcode=157,
            route=2,
            waits_for=255,
            cycles=4,
            value=18446744073709551605,
        )
    return ExpectedView(available=available, head=head)


@system
def routed_dependency_pipeline():
    epoch: u16 = 0
    inputs = four_routes_live_dependencies_stimulus(epoch)
    result = pipeline(inputs.valid, inputs.data, inputs.take)
    expected_ready = four_routes_live_dependencies_ready(epoch)
    expected_output = four_routes_live_dependencies_output(epoch)
    expected_scheduled = four_routes_live_dependencies_scheduled(epoch)
    expected_route_1_done = four_routes_live_dependencies_route_1_done(epoch)
    expected_route_3_done = four_routes_live_dependencies_route_3_done(epoch)
    expected_completed = four_routes_live_dependencies_completed(epoch)
    observe(
        epoch,
        result,
        expected_ready,
        expected_output,
        expected_scheduled,
        expected_route_1_done,
        expected_route_3_done,
        expected_completed,
    )


@rule
def full_topology_backpressure_stimulus(epoch: u16) -> Stimulus:
    valid: u1 = 0
    take: u1 = 0
    data = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 0:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=0,
            opcode=0,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551615,
        )
    if epoch == 1:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=1,
            opcode=17,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551614,
        )
    if epoch == 2:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=2,
            opcode=34,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 3:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=3,
            opcode=51,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551612,
        )
    if epoch == 4:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=4,
            opcode=68,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551611,
        )
    if epoch == 5:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=5,
            opcode=85,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551610,
        )
    if epoch == 6:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=6,
            opcode=102,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 7:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=7,
            opcode=119,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551608,
        )
    if epoch == 8:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=8,
            opcode=136,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551607,
        )
    if epoch == 9:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=9,
            opcode=153,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551606,
        )
    if epoch == 10:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=10,
            opcode=170,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 11:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=11,
            opcode=187,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551604,
        )
    if epoch == 12:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=12,
            opcode=204,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551603,
        )
    if epoch == 13:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=13,
            opcode=221,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551602,
        )
    if epoch == 14:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=14,
            opcode=238,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 15:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=15,
            opcode=255,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551600,
        )
    if epoch == 16:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=16,
            opcode=16,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551599,
        )
    if epoch == 17:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=17,
            opcode=33,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551598,
        )
    if epoch == 18:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=18,
            opcode=50,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 19:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=19,
            opcode=67,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551596,
        )
    if epoch == 20:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=20,
            opcode=84,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551595,
        )
    if epoch == 21:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=21,
            opcode=101,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551594,
        )
    if epoch == 22:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=22,
            opcode=118,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 23:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=23,
            opcode=135,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551592,
        )
    if epoch == 24:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=24,
            opcode=152,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551591,
        )
    if epoch == 25:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=25,
            opcode=169,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551590,
        )
    if epoch == 26:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=26,
            opcode=186,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 27:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=27,
            opcode=203,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551588,
        )
    if epoch == 28:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=28,
            opcode=220,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551587,
        )
    if epoch == 29:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=29,
            opcode=237,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551586,
        )
    if epoch == 30:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=30,
            opcode=254,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 31:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=31,
            opcode=15,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551584,
        )
    if epoch == 32:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=32,
            opcode=32,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551583,
        )
    if epoch == 33:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=33,
            opcode=49,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551582,
        )
    if epoch == 34:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=34,
            opcode=66,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 35:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=35,
            opcode=83,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551580,
        )
    if epoch == 36:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=36,
            opcode=100,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551579,
        )
    if epoch == 37:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=37,
            opcode=117,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551578,
        )
    if epoch == 38:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=38,
            opcode=134,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 39:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=39,
            opcode=151,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551576,
        )
    if epoch == 40:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=40,
            opcode=168,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551575,
        )
    if epoch == 41:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=41,
            opcode=185,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551574,
        )
    if epoch == 42:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=42,
            opcode=202,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 43:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=43,
            opcode=219,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551572,
        )
    if epoch == 44:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=44,
            opcode=236,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551571,
        )
    if epoch == 45:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=45,
            opcode=253,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551570,
        )
    if epoch == 46:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=46,
            opcode=14,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 47:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=47,
            opcode=31,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551568,
        )
    if epoch == 48:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=48,
            opcode=48,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551567,
        )
    if epoch == 49:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=49,
            opcode=65,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551566,
        )
    if epoch == 50:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=50,
            opcode=82,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 51:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=51,
            opcode=99,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551564,
        )
    if epoch == 52:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=52,
            opcode=116,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551563,
        )
    if epoch == 53:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=53,
            opcode=133,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551562,
        )
    if epoch == 54:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=54,
            opcode=150,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 55:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=55,
            opcode=167,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551560,
        )
    if epoch == 56:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=56,
            opcode=184,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551559,
        )
    if epoch == 57:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=57,
            opcode=201,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551558,
        )
    if epoch == 58:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=58,
            opcode=218,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 59:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=59,
            opcode=235,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551556,
        )
    if epoch == 60:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=60,
            opcode=252,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551555,
        )
    if epoch == 61:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=61,
            opcode=13,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551554,
        )
    if epoch == 62:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=62,
            opcode=30,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 63:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=63,
            opcode=47,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551552,
        )
    if epoch == 64:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=64,
            opcode=64,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551551,
        )
    if epoch == 65:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=65,
            opcode=81,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551550,
        )
    if epoch == 66:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=66,
            opcode=98,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 67:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=67,
            opcode=115,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551548,
        )
    if epoch == 68:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=68,
            opcode=132,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551547,
        )
    if epoch == 69:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=69,
            opcode=149,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551546,
        )
    if epoch == 70:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=70,
            opcode=166,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 71:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=71,
            opcode=183,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551544,
        )
    if epoch == 72:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=72,
            opcode=200,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551543,
        )
    if epoch == 73:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=73,
            opcode=217,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551542,
        )
    if epoch == 74:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=74,
            opcode=234,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch == 75:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=75,
            opcode=251,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551540,
        )
    if epoch == 76:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=76,
            opcode=12,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551539,
        )
    if epoch == 77:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=77,
            opcode=29,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551538,
        )
    if epoch == 78:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=78,
            opcode=46,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch == 79:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=79,
            opcode=63,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551536,
        )
    if epoch == 80:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=80,
            opcode=80,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551535,
        )
    if epoch == 81:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=81,
            opcode=97,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551534,
        )
    if epoch == 82:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=82,
            opcode=114,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch == 83:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=83,
            opcode=131,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551532,
        )
    if epoch == 84:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=84,
            opcode=148,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551531,
        )
    if epoch == 85:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=85,
            opcode=165,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551530,
        )
    if epoch == 86:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=86,
            opcode=182,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch == 87:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=87,
            opcode=199,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551528,
        )
    if epoch == 88:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=88,
            opcode=216,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551527,
        )
    if epoch == 89:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=89,
            opcode=233,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551526,
        )
    if epoch == 90:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=90,
            opcode=250,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch == 91:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=91,
            opcode=11,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551524,
        )
    if epoch == 92:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=92,
            opcode=28,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551523,
        )
    if epoch == 93:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=93,
            opcode=45,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551522,
        )
    if epoch == 94:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=94,
            opcode=62,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch == 95:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=95,
            opcode=79,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551520,
        )
    if epoch == 96:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=96,
            opcode=96,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551519,
        )
    if epoch == 97:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=97,
            opcode=113,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551518,
        )
    if epoch == 98:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=98,
            opcode=130,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch == 99:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=99,
            opcode=147,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551516,
        )
    if epoch == 100:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=100,
            opcode=164,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551515,
        )
    if epoch == 101:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=101,
            opcode=181,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551514,
        )
    if epoch == 102:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=102,
            opcode=198,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch == 103:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=103,
            opcode=215,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551512,
        )
    if epoch == 104:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=104,
            opcode=232,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551511,
        )
    if epoch == 105:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=105,
            opcode=249,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551510,
        )
    if epoch == 106:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=106,
            opcode=10,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch == 107:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=107,
            opcode=27,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551508,
        )
    if epoch == 108:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=108,
            opcode=44,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551507,
        )
    if epoch == 109:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=109,
            opcode=61,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551506,
        )
    if epoch == 110:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=110,
            opcode=78,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch == 111:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=111,
            opcode=95,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551504,
        )
    if epoch == 112:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=112,
            opcode=112,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551503,
        )
    if epoch == 113:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=113,
            opcode=129,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551502,
        )
    if epoch == 114:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=114,
            opcode=146,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch == 115:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=115,
            opcode=163,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551500,
        )
    if epoch == 116:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=116,
            opcode=180,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551499,
        )
    if epoch == 117:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=117,
            opcode=197,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551498,
        )
    if epoch == 118:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=118,
            opcode=214,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch == 119:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=119,
            opcode=231,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551496,
        )
    if epoch == 120:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=120,
            opcode=248,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551495,
        )
    if epoch == 121:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=121,
            opcode=9,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551494,
        )
    if epoch == 122:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=122,
            opcode=26,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch == 123:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=123,
            opcode=43,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551492,
        )
    if epoch == 124:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=124,
            opcode=60,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551491,
        )
    if epoch == 125:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=125,
            opcode=77,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551490,
        )
    if epoch == 126:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=126,
            opcode=94,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch == 127:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=127,
            opcode=111,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551488,
        )
    if epoch == 128:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=128,
            opcode=128,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551487,
        )
    if epoch == 129:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=129,
            opcode=145,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551486,
        )
    if epoch == 130:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=130,
            opcode=162,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch == 131:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=131,
            opcode=179,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551484,
        )
    if epoch == 132:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=132,
            opcode=196,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551483,
        )
    if epoch == 133:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=133,
            opcode=213,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551482,
        )
    if epoch == 134:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=134,
            opcode=230,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch == 135:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=135,
            opcode=247,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551480,
        )
    if epoch == 136:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=136,
            opcode=8,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551479,
        )
    if epoch == 137:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=137,
            opcode=25,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551478,
        )
    if epoch == 138:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=138,
            opcode=42,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch == 139:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=139,
            opcode=59,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551476,
        )
    if epoch == 140:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=140,
            opcode=76,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551475,
        )
    if epoch == 141:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=141,
            opcode=93,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551474,
        )
    if epoch == 142:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=142,
            opcode=110,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch == 143:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=143,
            opcode=127,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551472,
        )
    if epoch == 144:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=144,
            opcode=144,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551471,
        )
    if epoch == 145:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=145,
            opcode=161,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551470,
        )
    if epoch == 146:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=146,
            opcode=178,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch == 147:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=147,
            opcode=195,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551468,
        )
    if epoch == 148:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=148,
            opcode=212,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551467,
        )
    if epoch == 149:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=149,
            opcode=229,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551466,
        )
    if epoch == 150:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=150,
            opcode=246,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch == 151:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=151,
            opcode=7,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551464,
        )
    if epoch == 152:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=152,
            opcode=24,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551463,
        )
    if epoch == 153:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=153,
            opcode=41,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551462,
        )
    if epoch == 154:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=154,
            opcode=58,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch == 155:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=155,
            opcode=75,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551460,
        )
    if epoch == 156:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=156,
            opcode=92,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551459,
        )
    if epoch == 157:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=157,
            opcode=109,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551458,
        )
    if epoch == 158:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=158,
            opcode=126,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch == 159:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=159,
            opcode=143,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551456,
        )
    if epoch == 160:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=160,
            opcode=160,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551455,
        )
    if epoch >= 161 and epoch <= 359:
        valid = 0
        take = 0
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch >= 360 and epoch <= 363:
        valid = 1
        take = 0
        data = WorkItem(
            sequence_id=161,
            opcode=177,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551454,
        )
    if epoch >= 364 and epoch <= 373:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 374:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=161,
            opcode=177,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551454,
        )
    if epoch == 375:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 376:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=162,
            opcode=194,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch == 377:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 378:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=163,
            opcode=211,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551452,
        )
    if epoch == 379:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 380:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=164,
            opcode=228,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551451,
        )
    if epoch == 381:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 382:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=165,
            opcode=245,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551450,
        )
    if epoch == 383:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 384:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=166,
            opcode=6,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch == 385:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 386:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=167,
            opcode=23,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551448,
        )
    if epoch == 387:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 388:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=168,
            opcode=40,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551447,
        )
    if epoch == 389:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 390:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=169,
            opcode=57,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551446,
        )
    if epoch == 391:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 392:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=170,
            opcode=74,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch == 393:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 394:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=171,
            opcode=91,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551444,
        )
    if epoch == 395:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 396:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=172,
            opcode=108,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551443,
        )
    if epoch == 397:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 398:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=173,
            opcode=125,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551442,
        )
    if epoch == 399:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 400:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=174,
            opcode=142,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    if epoch == 401:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 402:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=175,
            opcode=159,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551440,
        )
    if epoch == 403:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 404:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=176,
            opcode=176,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551439,
        )
    if epoch == 405:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 406:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=177,
            opcode=193,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551438,
        )
    if epoch == 407:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 408:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=178,
            opcode=210,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551437,
        )
    if epoch == 409:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    if epoch == 410:
        valid = 1
        take = 1
        data = WorkItem(
            sequence_id=179,
            opcode=227,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551436,
        )
    if epoch >= 411 and epoch <= 726:
        valid = 0
        take = 1
        data = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0
        )
    return Stimulus(valid=valid, take=take, data=data)


@rule
def full_topology_backpressure_ready(epoch: u16) -> Ready:
    ready: u1 = 0
    if epoch >= 0 and epoch <= 160:
        ready = 1
    if epoch == 374:
        ready = 1
    if epoch == 376:
        ready = 1
    if epoch == 378:
        ready = 1
    if epoch == 380:
        ready = 1
    if epoch == 382:
        ready = 1
    if epoch == 384:
        ready = 1
    if epoch == 386:
        ready = 1
    if epoch == 388:
        ready = 1
    if epoch == 390:
        ready = 1
    if epoch == 392:
        ready = 1
    if epoch == 394:
        ready = 1
    if epoch == 396:
        ready = 1
    if epoch == 398:
        ready = 1
    if epoch == 400:
        ready = 1
    if epoch == 402:
        ready = 1
    if epoch == 404:
        ready = 1
    if epoch == 406:
        ready = 1
    if epoch == 408:
        ready = 1
    if epoch == 410:
        ready = 1
    if epoch >= 412 and epoch <= 726:
        ready = 1
    return Ready(value=ready)


@rule
def full_topology_backpressure_output(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch >= 12 and epoch <= 364:
        available = 1
        head = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=255, cycles=1, value=101
        )
    if epoch == 366:
        available = 1
        head = WorkItem(
            sequence_id=1, opcode=17, route=1, waits_for=255, cycles=1, value=101
        )
    if epoch == 368:
        available = 1
        head = WorkItem(
            sequence_id=2, opcode=34, route=2, waits_for=255, cycles=1, value=101
        )
    if epoch == 370:
        available = 1
        head = WorkItem(
            sequence_id=3, opcode=51, route=3, waits_for=255, cycles=1, value=101
        )
    if epoch == 372:
        available = 1
        head = WorkItem(
            sequence_id=4, opcode=68, route=0, waits_for=255, cycles=1, value=97
        )
    if epoch == 374:
        available = 1
        head = WorkItem(
            sequence_id=5, opcode=85, route=1, waits_for=255, cycles=1, value=97
        )
    if epoch == 376:
        available = 1
        head = WorkItem(
            sequence_id=6, opcode=102, route=2, waits_for=255, cycles=1, value=97
        )
    if epoch == 378:
        available = 1
        head = WorkItem(
            sequence_id=7, opcode=119, route=3, waits_for=255, cycles=1, value=97
        )
    if epoch == 380:
        available = 1
        head = WorkItem(
            sequence_id=8, opcode=136, route=0, waits_for=255, cycles=1, value=93
        )
    if epoch == 382:
        available = 1
        head = WorkItem(
            sequence_id=9, opcode=153, route=1, waits_for=255, cycles=1, value=93
        )
    if epoch == 384:
        available = 1
        head = WorkItem(
            sequence_id=10, opcode=170, route=2, waits_for=255, cycles=1, value=93
        )
    if epoch == 386:
        available = 1
        head = WorkItem(
            sequence_id=11, opcode=187, route=3, waits_for=255, cycles=1, value=93
        )
    if epoch == 388:
        available = 1
        head = WorkItem(
            sequence_id=12, opcode=204, route=0, waits_for=255, cycles=1, value=89
        )
    if epoch == 390:
        available = 1
        head = WorkItem(
            sequence_id=13, opcode=221, route=1, waits_for=255, cycles=1, value=89
        )
    if epoch == 392:
        available = 1
        head = WorkItem(
            sequence_id=14, opcode=238, route=2, waits_for=255, cycles=1, value=89
        )
    if epoch == 394:
        available = 1
        head = WorkItem(
            sequence_id=15, opcode=255, route=3, waits_for=255, cycles=1, value=89
        )
    if epoch == 396:
        available = 1
        head = WorkItem(
            sequence_id=16, opcode=16, route=0, waits_for=255, cycles=1, value=85
        )
    if epoch == 398:
        available = 1
        head = WorkItem(
            sequence_id=17, opcode=33, route=1, waits_for=255, cycles=1, value=85
        )
    if epoch == 400:
        available = 1
        head = WorkItem(
            sequence_id=18, opcode=50, route=2, waits_for=255, cycles=1, value=85
        )
    if epoch == 402:
        available = 1
        head = WorkItem(
            sequence_id=19, opcode=67, route=3, waits_for=255, cycles=1, value=85
        )
    if epoch == 404:
        available = 1
        head = WorkItem(
            sequence_id=20, opcode=84, route=0, waits_for=255, cycles=1, value=81
        )
    if epoch == 406:
        available = 1
        head = WorkItem(
            sequence_id=21, opcode=101, route=1, waits_for=255, cycles=1, value=81
        )
    if epoch == 408:
        available = 1
        head = WorkItem(
            sequence_id=22, opcode=118, route=2, waits_for=255, cycles=1, value=81
        )
    if epoch == 410:
        available = 1
        head = WorkItem(
            sequence_id=23, opcode=135, route=3, waits_for=255, cycles=1, value=81
        )
    if epoch == 412:
        available = 1
        head = WorkItem(
            sequence_id=24, opcode=152, route=0, waits_for=255, cycles=1, value=77
        )
    if epoch == 414:
        available = 1
        head = WorkItem(
            sequence_id=25, opcode=169, route=1, waits_for=255, cycles=1, value=77
        )
    if epoch == 416:
        available = 1
        head = WorkItem(
            sequence_id=26, opcode=186, route=2, waits_for=255, cycles=1, value=77
        )
    if epoch == 418:
        available = 1
        head = WorkItem(
            sequence_id=27, opcode=203, route=3, waits_for=255, cycles=1, value=77
        )
    if epoch == 420:
        available = 1
        head = WorkItem(
            sequence_id=28, opcode=220, route=0, waits_for=255, cycles=1, value=73
        )
    if epoch == 422:
        available = 1
        head = WorkItem(
            sequence_id=29, opcode=237, route=1, waits_for=255, cycles=1, value=73
        )
    if epoch == 424:
        available = 1
        head = WorkItem(
            sequence_id=30, opcode=254, route=2, waits_for=255, cycles=1, value=73
        )
    if epoch == 426:
        available = 1
        head = WorkItem(
            sequence_id=31, opcode=15, route=3, waits_for=255, cycles=1, value=73
        )
    if epoch == 428:
        available = 1
        head = WorkItem(
            sequence_id=32, opcode=32, route=0, waits_for=255, cycles=1, value=69
        )
    if epoch == 430:
        available = 1
        head = WorkItem(
            sequence_id=33, opcode=49, route=1, waits_for=255, cycles=1, value=69
        )
    if epoch == 432:
        available = 1
        head = WorkItem(
            sequence_id=34, opcode=66, route=2, waits_for=255, cycles=1, value=69
        )
    if epoch == 434:
        available = 1
        head = WorkItem(
            sequence_id=35, opcode=83, route=3, waits_for=255, cycles=1, value=69
        )
    if epoch == 436:
        available = 1
        head = WorkItem(
            sequence_id=36, opcode=100, route=0, waits_for=255, cycles=1, value=65
        )
    if epoch == 438:
        available = 1
        head = WorkItem(
            sequence_id=37, opcode=117, route=1, waits_for=255, cycles=1, value=65
        )
    if epoch == 440:
        available = 1
        head = WorkItem(
            sequence_id=38, opcode=134, route=2, waits_for=255, cycles=1, value=65
        )
    if epoch == 442:
        available = 1
        head = WorkItem(
            sequence_id=39, opcode=151, route=3, waits_for=255, cycles=1, value=65
        )
    if epoch == 444:
        available = 1
        head = WorkItem(
            sequence_id=40, opcode=168, route=0, waits_for=255, cycles=1, value=61
        )
    if epoch == 446:
        available = 1
        head = WorkItem(
            sequence_id=41, opcode=185, route=1, waits_for=255, cycles=1, value=61
        )
    if epoch == 448:
        available = 1
        head = WorkItem(
            sequence_id=42, opcode=202, route=2, waits_for=255, cycles=1, value=61
        )
    if epoch == 450:
        available = 1
        head = WorkItem(
            sequence_id=43, opcode=219, route=3, waits_for=255, cycles=1, value=61
        )
    if epoch == 452:
        available = 1
        head = WorkItem(
            sequence_id=44, opcode=236, route=0, waits_for=255, cycles=1, value=57
        )
    if epoch == 454:
        available = 1
        head = WorkItem(
            sequence_id=45, opcode=253, route=1, waits_for=255, cycles=1, value=57
        )
    if epoch == 456:
        available = 1
        head = WorkItem(
            sequence_id=46, opcode=14, route=2, waits_for=255, cycles=1, value=57
        )
    if epoch == 458:
        available = 1
        head = WorkItem(
            sequence_id=47, opcode=31, route=3, waits_for=255, cycles=1, value=57
        )
    if epoch == 460:
        available = 1
        head = WorkItem(
            sequence_id=48, opcode=48, route=0, waits_for=255, cycles=1, value=53
        )
    if epoch == 462:
        available = 1
        head = WorkItem(
            sequence_id=49, opcode=65, route=1, waits_for=255, cycles=1, value=53
        )
    if epoch == 464:
        available = 1
        head = WorkItem(
            sequence_id=50, opcode=82, route=2, waits_for=255, cycles=1, value=53
        )
    if epoch == 466:
        available = 1
        head = WorkItem(
            sequence_id=51, opcode=99, route=3, waits_for=255, cycles=1, value=53
        )
    if epoch == 468:
        available = 1
        head = WorkItem(
            sequence_id=52, opcode=116, route=0, waits_for=255, cycles=1, value=49
        )
    if epoch == 470:
        available = 1
        head = WorkItem(
            sequence_id=53, opcode=133, route=1, waits_for=255, cycles=1, value=49
        )
    if epoch == 472:
        available = 1
        head = WorkItem(
            sequence_id=54, opcode=150, route=2, waits_for=255, cycles=1, value=49
        )
    if epoch == 474:
        available = 1
        head = WorkItem(
            sequence_id=55, opcode=167, route=3, waits_for=255, cycles=1, value=49
        )
    if epoch == 476:
        available = 1
        head = WorkItem(
            sequence_id=56, opcode=184, route=0, waits_for=255, cycles=1, value=45
        )
    if epoch == 478:
        available = 1
        head = WorkItem(
            sequence_id=57, opcode=201, route=1, waits_for=255, cycles=1, value=45
        )
    if epoch == 480:
        available = 1
        head = WorkItem(
            sequence_id=58, opcode=218, route=2, waits_for=255, cycles=1, value=45
        )
    if epoch == 482:
        available = 1
        head = WorkItem(
            sequence_id=59, opcode=235, route=3, waits_for=255, cycles=1, value=45
        )
    if epoch == 484:
        available = 1
        head = WorkItem(
            sequence_id=60, opcode=252, route=0, waits_for=255, cycles=1, value=41
        )
    if epoch == 486:
        available = 1
        head = WorkItem(
            sequence_id=61, opcode=13, route=1, waits_for=255, cycles=1, value=41
        )
    if epoch == 488:
        available = 1
        head = WorkItem(
            sequence_id=62, opcode=30, route=2, waits_for=255, cycles=1, value=41
        )
    if epoch == 490:
        available = 1
        head = WorkItem(
            sequence_id=63, opcode=47, route=3, waits_for=255, cycles=1, value=41
        )
    if epoch == 492:
        available = 1
        head = WorkItem(
            sequence_id=64, opcode=64, route=0, waits_for=255, cycles=1, value=37
        )
    if epoch == 494:
        available = 1
        head = WorkItem(
            sequence_id=65, opcode=81, route=1, waits_for=255, cycles=1, value=37
        )
    if epoch == 496:
        available = 1
        head = WorkItem(
            sequence_id=66, opcode=98, route=2, waits_for=255, cycles=1, value=37
        )
    if epoch == 498:
        available = 1
        head = WorkItem(
            sequence_id=67, opcode=115, route=3, waits_for=255, cycles=1, value=37
        )
    if epoch == 500:
        available = 1
        head = WorkItem(
            sequence_id=68, opcode=132, route=0, waits_for=255, cycles=1, value=33
        )
    if epoch == 502:
        available = 1
        head = WorkItem(
            sequence_id=69, opcode=149, route=1, waits_for=255, cycles=1, value=33
        )
    if epoch == 504:
        available = 1
        head = WorkItem(
            sequence_id=70, opcode=166, route=2, waits_for=255, cycles=1, value=33
        )
    if epoch == 506:
        available = 1
        head = WorkItem(
            sequence_id=71, opcode=183, route=3, waits_for=255, cycles=1, value=33
        )
    if epoch == 508:
        available = 1
        head = WorkItem(
            sequence_id=72, opcode=200, route=0, waits_for=255, cycles=1, value=29
        )
    if epoch == 510:
        available = 1
        head = WorkItem(
            sequence_id=73, opcode=217, route=1, waits_for=255, cycles=1, value=29
        )
    if epoch == 512:
        available = 1
        head = WorkItem(
            sequence_id=74, opcode=234, route=2, waits_for=255, cycles=1, value=29
        )
    if epoch == 514:
        available = 1
        head = WorkItem(
            sequence_id=75, opcode=251, route=3, waits_for=255, cycles=1, value=29
        )
    if epoch == 516:
        available = 1
        head = WorkItem(
            sequence_id=76, opcode=12, route=0, waits_for=255, cycles=1, value=25
        )
    if epoch == 518:
        available = 1
        head = WorkItem(
            sequence_id=77, opcode=29, route=1, waits_for=255, cycles=1, value=25
        )
    if epoch == 520:
        available = 1
        head = WorkItem(
            sequence_id=78, opcode=46, route=2, waits_for=255, cycles=1, value=25
        )
    if epoch == 522:
        available = 1
        head = WorkItem(
            sequence_id=79, opcode=63, route=3, waits_for=255, cycles=1, value=25
        )
    if epoch == 524:
        available = 1
        head = WorkItem(
            sequence_id=80, opcode=80, route=0, waits_for=255, cycles=1, value=21
        )
    if epoch == 526:
        available = 1
        head = WorkItem(
            sequence_id=81, opcode=97, route=1, waits_for=255, cycles=1, value=21
        )
    if epoch == 528:
        available = 1
        head = WorkItem(
            sequence_id=82, opcode=114, route=2, waits_for=255, cycles=1, value=21
        )
    if epoch == 530:
        available = 1
        head = WorkItem(
            sequence_id=83, opcode=131, route=3, waits_for=255, cycles=1, value=21
        )
    if epoch == 532:
        available = 1
        head = WorkItem(
            sequence_id=84, opcode=148, route=0, waits_for=255, cycles=1, value=17
        )
    if epoch == 534:
        available = 1
        head = WorkItem(
            sequence_id=85, opcode=165, route=1, waits_for=255, cycles=1, value=17
        )
    if epoch == 536:
        available = 1
        head = WorkItem(
            sequence_id=86, opcode=182, route=2, waits_for=255, cycles=1, value=17
        )
    if epoch == 538:
        available = 1
        head = WorkItem(
            sequence_id=87, opcode=199, route=3, waits_for=255, cycles=1, value=17
        )
    if epoch == 540:
        available = 1
        head = WorkItem(
            sequence_id=88, opcode=216, route=0, waits_for=255, cycles=1, value=13
        )
    if epoch == 542:
        available = 1
        head = WorkItem(
            sequence_id=89, opcode=233, route=1, waits_for=255, cycles=1, value=13
        )
    if epoch == 544:
        available = 1
        head = WorkItem(
            sequence_id=90, opcode=250, route=2, waits_for=255, cycles=1, value=13
        )
    if epoch == 546:
        available = 1
        head = WorkItem(
            sequence_id=91, opcode=11, route=3, waits_for=255, cycles=1, value=13
        )
    if epoch == 548:
        available = 1
        head = WorkItem(
            sequence_id=92, opcode=28, route=0, waits_for=255, cycles=1, value=9
        )
    if epoch == 550:
        available = 1
        head = WorkItem(
            sequence_id=93, opcode=45, route=1, waits_for=255, cycles=1, value=9
        )
    if epoch == 552:
        available = 1
        head = WorkItem(
            sequence_id=94, opcode=62, route=2, waits_for=255, cycles=1, value=9
        )
    if epoch == 554:
        available = 1
        head = WorkItem(
            sequence_id=95, opcode=79, route=3, waits_for=255, cycles=1, value=9
        )
    if epoch == 556:
        available = 1
        head = WorkItem(
            sequence_id=96, opcode=96, route=0, waits_for=255, cycles=1, value=5
        )
    if epoch == 558:
        available = 1
        head = WorkItem(
            sequence_id=97, opcode=113, route=1, waits_for=255, cycles=1, value=5
        )
    if epoch == 560:
        available = 1
        head = WorkItem(
            sequence_id=98, opcode=130, route=2, waits_for=255, cycles=1, value=5
        )
    if epoch == 562:
        available = 1
        head = WorkItem(
            sequence_id=99, opcode=147, route=3, waits_for=255, cycles=1, value=5
        )
    if epoch == 564:
        available = 1
        head = WorkItem(
            sequence_id=100, opcode=164, route=0, waits_for=255, cycles=1, value=1
        )
    if epoch == 566:
        available = 1
        head = WorkItem(
            sequence_id=101, opcode=181, route=1, waits_for=255, cycles=1, value=1
        )
    if epoch == 568:
        available = 1
        head = WorkItem(
            sequence_id=102, opcode=198, route=2, waits_for=255, cycles=1, value=1
        )
    if epoch == 570:
        available = 1
        head = WorkItem(
            sequence_id=103, opcode=215, route=3, waits_for=255, cycles=1, value=1
        )
    if epoch == 572:
        available = 1
        head = WorkItem(
            sequence_id=104,
            opcode=232,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 574:
        available = 1
        head = WorkItem(
            sequence_id=105,
            opcode=249,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 576:
        available = 1
        head = WorkItem(
            sequence_id=106,
            opcode=10,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 578:
        available = 1
        head = WorkItem(
            sequence_id=107,
            opcode=27,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 580:
        available = 1
        head = WorkItem(
            sequence_id=108,
            opcode=44,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 582:
        available = 1
        head = WorkItem(
            sequence_id=109,
            opcode=61,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 584:
        available = 1
        head = WorkItem(
            sequence_id=110,
            opcode=78,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 586:
        available = 1
        head = WorkItem(
            sequence_id=111,
            opcode=95,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 588:
        available = 1
        head = WorkItem(
            sequence_id=112,
            opcode=112,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 590:
        available = 1
        head = WorkItem(
            sequence_id=113,
            opcode=129,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 592:
        available = 1
        head = WorkItem(
            sequence_id=114,
            opcode=146,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 594:
        available = 1
        head = WorkItem(
            sequence_id=115,
            opcode=163,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 596:
        available = 1
        head = WorkItem(
            sequence_id=116,
            opcode=180,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 598:
        available = 1
        head = WorkItem(
            sequence_id=117,
            opcode=197,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 600:
        available = 1
        head = WorkItem(
            sequence_id=118,
            opcode=214,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 602:
        available = 1
        head = WorkItem(
            sequence_id=119,
            opcode=231,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 604:
        available = 1
        head = WorkItem(
            sequence_id=120,
            opcode=248,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 606:
        available = 1
        head = WorkItem(
            sequence_id=121,
            opcode=9,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 608:
        available = 1
        head = WorkItem(
            sequence_id=122,
            opcode=26,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 610:
        available = 1
        head = WorkItem(
            sequence_id=123,
            opcode=43,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 612:
        available = 1
        head = WorkItem(
            sequence_id=124,
            opcode=60,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 614:
        available = 1
        head = WorkItem(
            sequence_id=125,
            opcode=77,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 616:
        available = 1
        head = WorkItem(
            sequence_id=126,
            opcode=94,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 618:
        available = 1
        head = WorkItem(
            sequence_id=127,
            opcode=111,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 620:
        available = 1
        head = WorkItem(
            sequence_id=128,
            opcode=128,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 622:
        available = 1
        head = WorkItem(
            sequence_id=129,
            opcode=145,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 624:
        available = 1
        head = WorkItem(
            sequence_id=130,
            opcode=162,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 626:
        available = 1
        head = WorkItem(
            sequence_id=131,
            opcode=179,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 628:
        available = 1
        head = WorkItem(
            sequence_id=132,
            opcode=196,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 630:
        available = 1
        head = WorkItem(
            sequence_id=133,
            opcode=213,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 632:
        available = 1
        head = WorkItem(
            sequence_id=134,
            opcode=230,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 634:
        available = 1
        head = WorkItem(
            sequence_id=135,
            opcode=247,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 636:
        available = 1
        head = WorkItem(
            sequence_id=136,
            opcode=8,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 638:
        available = 1
        head = WorkItem(
            sequence_id=137,
            opcode=25,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 640:
        available = 1
        head = WorkItem(
            sequence_id=138,
            opcode=42,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 642:
        available = 1
        head = WorkItem(
            sequence_id=139,
            opcode=59,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 644:
        available = 1
        head = WorkItem(
            sequence_id=140,
            opcode=76,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 646:
        available = 1
        head = WorkItem(
            sequence_id=141,
            opcode=93,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 648:
        available = 1
        head = WorkItem(
            sequence_id=142,
            opcode=110,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 650:
        available = 1
        head = WorkItem(
            sequence_id=143,
            opcode=127,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 652:
        available = 1
        head = WorkItem(
            sequence_id=144,
            opcode=144,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 654:
        available = 1
        head = WorkItem(
            sequence_id=145,
            opcode=161,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 656:
        available = 1
        head = WorkItem(
            sequence_id=146,
            opcode=178,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 658:
        available = 1
        head = WorkItem(
            sequence_id=147,
            opcode=195,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 660:
        available = 1
        head = WorkItem(
            sequence_id=148,
            opcode=212,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 662:
        available = 1
        head = WorkItem(
            sequence_id=149,
            opcode=229,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 664:
        available = 1
        head = WorkItem(
            sequence_id=150,
            opcode=246,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 666:
        available = 1
        head = WorkItem(
            sequence_id=151,
            opcode=7,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 668:
        available = 1
        head = WorkItem(
            sequence_id=152,
            opcode=24,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 670:
        available = 1
        head = WorkItem(
            sequence_id=153,
            opcode=41,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 672:
        available = 1
        head = WorkItem(
            sequence_id=154,
            opcode=58,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 674:
        available = 1
        head = WorkItem(
            sequence_id=155,
            opcode=75,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 676:
        available = 1
        head = WorkItem(
            sequence_id=156,
            opcode=92,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 678:
        available = 1
        head = WorkItem(
            sequence_id=157,
            opcode=109,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 680:
        available = 1
        head = WorkItem(
            sequence_id=158,
            opcode=126,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 682:
        available = 1
        head = WorkItem(
            sequence_id=159,
            opcode=143,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 684:
        available = 1
        head = WorkItem(
            sequence_id=160,
            opcode=160,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 686:
        available = 1
        head = WorkItem(
            sequence_id=161,
            opcode=177,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 688:
        available = 1
        head = WorkItem(
            sequence_id=162,
            opcode=194,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 690:
        available = 1
        head = WorkItem(
            sequence_id=163,
            opcode=211,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 692:
        available = 1
        head = WorkItem(
            sequence_id=164,
            opcode=228,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 694:
        available = 1
        head = WorkItem(
            sequence_id=165,
            opcode=245,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 696:
        available = 1
        head = WorkItem(
            sequence_id=166,
            opcode=6,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 698:
        available = 1
        head = WorkItem(
            sequence_id=167,
            opcode=23,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 700:
        available = 1
        head = WorkItem(
            sequence_id=168,
            opcode=40,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 702:
        available = 1
        head = WorkItem(
            sequence_id=169,
            opcode=57,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 704:
        available = 1
        head = WorkItem(
            sequence_id=170,
            opcode=74,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 706:
        available = 1
        head = WorkItem(
            sequence_id=171,
            opcode=91,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 708:
        available = 1
        head = WorkItem(
            sequence_id=172,
            opcode=108,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 710:
        available = 1
        head = WorkItem(
            sequence_id=173,
            opcode=125,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 712:
        available = 1
        head = WorkItem(
            sequence_id=174,
            opcode=142,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 714:
        available = 1
        head = WorkItem(
            sequence_id=175,
            opcode=159,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 716:
        available = 1
        head = WorkItem(
            sequence_id=176,
            opcode=176,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch == 718:
        available = 1
        head = WorkItem(
            sequence_id=177,
            opcode=193,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch == 720:
        available = 1
        head = WorkItem(
            sequence_id=178,
            opcode=210,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch == 722:
        available = 1
        head = WorkItem(
            sequence_id=179,
            opcode=227,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    return ExpectedView(available=available, head=head)


@rule
def full_topology_backpressure_scheduled(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 6:
        available = 1
        head = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=255, cycles=1, value=0
        )
    if epoch == 7:
        available = 1
        head = WorkItem(
            sequence_id=1,
            opcode=17,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551615,
        )
    if epoch == 8:
        available = 1
        head = WorkItem(
            sequence_id=2,
            opcode=34,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551614,
        )
    if epoch == 9:
        available = 1
        head = WorkItem(
            sequence_id=3,
            opcode=51,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 10:
        available = 1
        head = WorkItem(
            sequence_id=4,
            opcode=68,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551612,
        )
    if epoch == 11:
        available = 1
        head = WorkItem(
            sequence_id=5,
            opcode=85,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551611,
        )
    if epoch == 12:
        available = 1
        head = WorkItem(
            sequence_id=6,
            opcode=102,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551610,
        )
    if epoch == 13:
        available = 1
        head = WorkItem(
            sequence_id=7,
            opcode=119,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 14:
        available = 1
        head = WorkItem(
            sequence_id=8,
            opcode=136,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551608,
        )
    if epoch == 15:
        available = 1
        head = WorkItem(
            sequence_id=9,
            opcode=153,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551607,
        )
    if epoch == 16:
        available = 1
        head = WorkItem(
            sequence_id=10,
            opcode=170,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551606,
        )
    if epoch == 17:
        available = 1
        head = WorkItem(
            sequence_id=11,
            opcode=187,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 18:
        available = 1
        head = WorkItem(
            sequence_id=12,
            opcode=204,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551604,
        )
    if epoch == 19:
        available = 1
        head = WorkItem(
            sequence_id=13,
            opcode=221,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551603,
        )
    if epoch == 20:
        available = 1
        head = WorkItem(
            sequence_id=14,
            opcode=238,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551602,
        )
    if epoch == 21:
        available = 1
        head = WorkItem(
            sequence_id=15,
            opcode=255,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 22:
        available = 1
        head = WorkItem(
            sequence_id=16,
            opcode=16,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551600,
        )
    if epoch == 23:
        available = 1
        head = WorkItem(
            sequence_id=17,
            opcode=33,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551599,
        )
    if epoch == 24:
        available = 1
        head = WorkItem(
            sequence_id=18,
            opcode=50,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551598,
        )
    if epoch == 25:
        available = 1
        head = WorkItem(
            sequence_id=19,
            opcode=67,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 26:
        available = 1
        head = WorkItem(
            sequence_id=20,
            opcode=84,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551596,
        )
    if epoch == 27:
        available = 1
        head = WorkItem(
            sequence_id=21,
            opcode=101,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551595,
        )
    if epoch == 28:
        available = 1
        head = WorkItem(
            sequence_id=22,
            opcode=118,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551594,
        )
    if epoch == 29:
        available = 1
        head = WorkItem(
            sequence_id=23,
            opcode=135,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 30:
        available = 1
        head = WorkItem(
            sequence_id=24,
            opcode=152,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551592,
        )
    if epoch == 31:
        available = 1
        head = WorkItem(
            sequence_id=25,
            opcode=169,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551591,
        )
    if epoch == 32:
        available = 1
        head = WorkItem(
            sequence_id=26,
            opcode=186,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551590,
        )
    if epoch == 33:
        available = 1
        head = WorkItem(
            sequence_id=27,
            opcode=203,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 34:
        available = 1
        head = WorkItem(
            sequence_id=28,
            opcode=220,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551588,
        )
    if epoch == 35:
        available = 1
        head = WorkItem(
            sequence_id=29,
            opcode=237,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551587,
        )
    if epoch == 36:
        available = 1
        head = WorkItem(
            sequence_id=30,
            opcode=254,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551586,
        )
    if epoch == 37:
        available = 1
        head = WorkItem(
            sequence_id=31,
            opcode=15,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 38:
        available = 1
        head = WorkItem(
            sequence_id=32,
            opcode=32,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551584,
        )
    if epoch == 39:
        available = 1
        head = WorkItem(
            sequence_id=33,
            opcode=49,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551583,
        )
    if epoch == 40:
        available = 1
        head = WorkItem(
            sequence_id=34,
            opcode=66,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551582,
        )
    if epoch == 41:
        available = 1
        head = WorkItem(
            sequence_id=35,
            opcode=83,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 42:
        available = 1
        head = WorkItem(
            sequence_id=36,
            opcode=100,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551580,
        )
    if epoch == 43:
        available = 1
        head = WorkItem(
            sequence_id=37,
            opcode=117,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551579,
        )
    if epoch == 44:
        available = 1
        head = WorkItem(
            sequence_id=38,
            opcode=134,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551578,
        )
    if epoch == 45:
        available = 1
        head = WorkItem(
            sequence_id=39,
            opcode=151,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 46:
        available = 1
        head = WorkItem(
            sequence_id=40,
            opcode=168,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551576,
        )
    if epoch == 47:
        available = 1
        head = WorkItem(
            sequence_id=41,
            opcode=185,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551575,
        )
    if epoch == 48:
        available = 1
        head = WorkItem(
            sequence_id=42,
            opcode=202,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551574,
        )
    if epoch == 49:
        available = 1
        head = WorkItem(
            sequence_id=43,
            opcode=219,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 50:
        available = 1
        head = WorkItem(
            sequence_id=44,
            opcode=236,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551572,
        )
    if epoch == 51:
        available = 1
        head = WorkItem(
            sequence_id=45,
            opcode=253,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551571,
        )
    if epoch == 52:
        available = 1
        head = WorkItem(
            sequence_id=46,
            opcode=14,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551570,
        )
    if epoch == 53:
        available = 1
        head = WorkItem(
            sequence_id=47,
            opcode=31,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 54:
        available = 1
        head = WorkItem(
            sequence_id=48,
            opcode=48,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551568,
        )
    if epoch == 55:
        available = 1
        head = WorkItem(
            sequence_id=49,
            opcode=65,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551567,
        )
    if epoch == 56:
        available = 1
        head = WorkItem(
            sequence_id=50,
            opcode=82,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551566,
        )
    if epoch == 57:
        available = 1
        head = WorkItem(
            sequence_id=51,
            opcode=99,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 58:
        available = 1
        head = WorkItem(
            sequence_id=52,
            opcode=116,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551564,
        )
    if epoch == 59:
        available = 1
        head = WorkItem(
            sequence_id=53,
            opcode=133,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551563,
        )
    if epoch == 60:
        available = 1
        head = WorkItem(
            sequence_id=54,
            opcode=150,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551562,
        )
    if epoch == 61:
        available = 1
        head = WorkItem(
            sequence_id=55,
            opcode=167,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 62:
        available = 1
        head = WorkItem(
            sequence_id=56,
            opcode=184,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551560,
        )
    if epoch == 63:
        available = 1
        head = WorkItem(
            sequence_id=57,
            opcode=201,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551559,
        )
    if epoch == 64:
        available = 1
        head = WorkItem(
            sequence_id=58,
            opcode=218,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551558,
        )
    if epoch == 65:
        available = 1
        head = WorkItem(
            sequence_id=59,
            opcode=235,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 66:
        available = 1
        head = WorkItem(
            sequence_id=60,
            opcode=252,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551556,
        )
    if epoch == 67:
        available = 1
        head = WorkItem(
            sequence_id=61,
            opcode=13,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551555,
        )
    if epoch == 68:
        available = 1
        head = WorkItem(
            sequence_id=62,
            opcode=30,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551554,
        )
    if epoch == 69:
        available = 1
        head = WorkItem(
            sequence_id=63,
            opcode=47,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 70:
        available = 1
        head = WorkItem(
            sequence_id=64,
            opcode=64,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551552,
        )
    if epoch == 71:
        available = 1
        head = WorkItem(
            sequence_id=65,
            opcode=81,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551551,
        )
    if epoch == 72:
        available = 1
        head = WorkItem(
            sequence_id=66,
            opcode=98,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551550,
        )
    if epoch == 73:
        available = 1
        head = WorkItem(
            sequence_id=67,
            opcode=115,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 74:
        available = 1
        head = WorkItem(
            sequence_id=68,
            opcode=132,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551548,
        )
    if epoch == 75:
        available = 1
        head = WorkItem(
            sequence_id=69,
            opcode=149,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551547,
        )
    if epoch == 76:
        available = 1
        head = WorkItem(
            sequence_id=70,
            opcode=166,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551546,
        )
    if epoch == 77:
        available = 1
        head = WorkItem(
            sequence_id=71,
            opcode=183,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 78:
        available = 1
        head = WorkItem(
            sequence_id=72,
            opcode=200,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551544,
        )
    if epoch == 79:
        available = 1
        head = WorkItem(
            sequence_id=73,
            opcode=217,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551543,
        )
    if epoch == 80:
        available = 1
        head = WorkItem(
            sequence_id=74,
            opcode=234,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551542,
        )
    if epoch == 81:
        available = 1
        head = WorkItem(
            sequence_id=75,
            opcode=251,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch == 82:
        available = 1
        head = WorkItem(
            sequence_id=76,
            opcode=12,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551540,
        )
    if epoch == 83:
        available = 1
        head = WorkItem(
            sequence_id=77,
            opcode=29,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551539,
        )
    if epoch == 84:
        available = 1
        head = WorkItem(
            sequence_id=78,
            opcode=46,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551538,
        )
    if epoch == 85:
        available = 1
        head = WorkItem(
            sequence_id=79,
            opcode=63,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch == 86:
        available = 1
        head = WorkItem(
            sequence_id=80,
            opcode=80,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551536,
        )
    if epoch == 87:
        available = 1
        head = WorkItem(
            sequence_id=81,
            opcode=97,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551535,
        )
    if epoch == 88:
        available = 1
        head = WorkItem(
            sequence_id=82,
            opcode=114,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551534,
        )
    if epoch == 89:
        available = 1
        head = WorkItem(
            sequence_id=83,
            opcode=131,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch == 90:
        available = 1
        head = WorkItem(
            sequence_id=84,
            opcode=148,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551532,
        )
    if epoch == 91:
        available = 1
        head = WorkItem(
            sequence_id=85,
            opcode=165,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551531,
        )
    if epoch == 92:
        available = 1
        head = WorkItem(
            sequence_id=86,
            opcode=182,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551530,
        )
    if epoch == 93:
        available = 1
        head = WorkItem(
            sequence_id=87,
            opcode=199,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch == 94:
        available = 1
        head = WorkItem(
            sequence_id=88,
            opcode=216,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551528,
        )
    if epoch == 95:
        available = 1
        head = WorkItem(
            sequence_id=89,
            opcode=233,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551527,
        )
    if epoch == 96:
        available = 1
        head = WorkItem(
            sequence_id=90,
            opcode=250,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551526,
        )
    if epoch == 97:
        available = 1
        head = WorkItem(
            sequence_id=91,
            opcode=11,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch == 98:
        available = 1
        head = WorkItem(
            sequence_id=92,
            opcode=28,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551524,
        )
    if epoch == 99:
        available = 1
        head = WorkItem(
            sequence_id=93,
            opcode=45,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551523,
        )
    if epoch == 100:
        available = 1
        head = WorkItem(
            sequence_id=94,
            opcode=62,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551522,
        )
    if epoch == 101:
        available = 1
        head = WorkItem(
            sequence_id=95,
            opcode=79,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch == 102:
        available = 1
        head = WorkItem(
            sequence_id=96,
            opcode=96,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551520,
        )
    if epoch == 103:
        available = 1
        head = WorkItem(
            sequence_id=97,
            opcode=113,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551519,
        )
    if epoch == 104:
        available = 1
        head = WorkItem(
            sequence_id=98,
            opcode=130,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551518,
        )
    if epoch == 105:
        available = 1
        head = WorkItem(
            sequence_id=99,
            opcode=147,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch == 106:
        available = 1
        head = WorkItem(
            sequence_id=100,
            opcode=164,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551516,
        )
    if epoch == 107:
        available = 1
        head = WorkItem(
            sequence_id=101,
            opcode=181,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551515,
        )
    if epoch == 108:
        available = 1
        head = WorkItem(
            sequence_id=102,
            opcode=198,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551514,
        )
    if epoch == 109:
        available = 1
        head = WorkItem(
            sequence_id=103,
            opcode=215,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch == 110:
        available = 1
        head = WorkItem(
            sequence_id=104,
            opcode=232,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551512,
        )
    if epoch == 111:
        available = 1
        head = WorkItem(
            sequence_id=105,
            opcode=249,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551511,
        )
    if epoch == 112:
        available = 1
        head = WorkItem(
            sequence_id=106,
            opcode=10,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551510,
        )
    if epoch == 113:
        available = 1
        head = WorkItem(
            sequence_id=107,
            opcode=27,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch == 114:
        available = 1
        head = WorkItem(
            sequence_id=108,
            opcode=44,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551508,
        )
    if epoch == 115:
        available = 1
        head = WorkItem(
            sequence_id=109,
            opcode=61,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551507,
        )
    if epoch == 116:
        available = 1
        head = WorkItem(
            sequence_id=110,
            opcode=78,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551506,
        )
    if epoch == 117:
        available = 1
        head = WorkItem(
            sequence_id=111,
            opcode=95,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch == 118:
        available = 1
        head = WorkItem(
            sequence_id=112,
            opcode=112,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551504,
        )
    if epoch == 119:
        available = 1
        head = WorkItem(
            sequence_id=113,
            opcode=129,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551503,
        )
    if epoch == 120:
        available = 1
        head = WorkItem(
            sequence_id=114,
            opcode=146,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551502,
        )
    if epoch == 121:
        available = 1
        head = WorkItem(
            sequence_id=115,
            opcode=163,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch == 122:
        available = 1
        head = WorkItem(
            sequence_id=116,
            opcode=180,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551500,
        )
    if epoch >= 123 and epoch <= 370:
        available = 1
        head = WorkItem(
            sequence_id=117,
            opcode=197,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551499,
        )
    if epoch >= 371 and epoch <= 372:
        available = 1
        head = WorkItem(
            sequence_id=118,
            opcode=214,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551498,
        )
    if epoch >= 373 and epoch <= 374:
        available = 1
        head = WorkItem(
            sequence_id=119,
            opcode=231,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch >= 375 and epoch <= 376:
        available = 1
        head = WorkItem(
            sequence_id=120,
            opcode=248,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551496,
        )
    if epoch >= 377 and epoch <= 378:
        available = 1
        head = WorkItem(
            sequence_id=121,
            opcode=9,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551495,
        )
    if epoch >= 379 and epoch <= 380:
        available = 1
        head = WorkItem(
            sequence_id=122,
            opcode=26,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551494,
        )
    if epoch >= 381 and epoch <= 382:
        available = 1
        head = WorkItem(
            sequence_id=123,
            opcode=43,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch >= 383 and epoch <= 384:
        available = 1
        head = WorkItem(
            sequence_id=124,
            opcode=60,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551492,
        )
    if epoch >= 385 and epoch <= 386:
        available = 1
        head = WorkItem(
            sequence_id=125,
            opcode=77,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551491,
        )
    if epoch >= 387 and epoch <= 388:
        available = 1
        head = WorkItem(
            sequence_id=126,
            opcode=94,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551490,
        )
    if epoch >= 389 and epoch <= 390:
        available = 1
        head = WorkItem(
            sequence_id=127,
            opcode=111,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch >= 391 and epoch <= 392:
        available = 1
        head = WorkItem(
            sequence_id=128,
            opcode=128,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551488,
        )
    if epoch >= 393 and epoch <= 394:
        available = 1
        head = WorkItem(
            sequence_id=129,
            opcode=145,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551487,
        )
    if epoch >= 395 and epoch <= 396:
        available = 1
        head = WorkItem(
            sequence_id=130,
            opcode=162,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551486,
        )
    if epoch >= 397 and epoch <= 398:
        available = 1
        head = WorkItem(
            sequence_id=131,
            opcode=179,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch >= 399 and epoch <= 400:
        available = 1
        head = WorkItem(
            sequence_id=132,
            opcode=196,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551484,
        )
    if epoch >= 401 and epoch <= 402:
        available = 1
        head = WorkItem(
            sequence_id=133,
            opcode=213,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551483,
        )
    if epoch >= 403 and epoch <= 404:
        available = 1
        head = WorkItem(
            sequence_id=134,
            opcode=230,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551482,
        )
    if epoch >= 405 and epoch <= 406:
        available = 1
        head = WorkItem(
            sequence_id=135,
            opcode=247,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch >= 407 and epoch <= 408:
        available = 1
        head = WorkItem(
            sequence_id=136,
            opcode=8,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551480,
        )
    if epoch >= 409 and epoch <= 410:
        available = 1
        head = WorkItem(
            sequence_id=137,
            opcode=25,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551479,
        )
    if epoch >= 411 and epoch <= 412:
        available = 1
        head = WorkItem(
            sequence_id=138,
            opcode=42,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551478,
        )
    if epoch >= 413 and epoch <= 414:
        available = 1
        head = WorkItem(
            sequence_id=139,
            opcode=59,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch >= 415 and epoch <= 416:
        available = 1
        head = WorkItem(
            sequence_id=140,
            opcode=76,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551476,
        )
    if epoch >= 417 and epoch <= 418:
        available = 1
        head = WorkItem(
            sequence_id=141,
            opcode=93,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551475,
        )
    if epoch >= 419 and epoch <= 420:
        available = 1
        head = WorkItem(
            sequence_id=142,
            opcode=110,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551474,
        )
    if epoch >= 421 and epoch <= 422:
        available = 1
        head = WorkItem(
            sequence_id=143,
            opcode=127,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch >= 423 and epoch <= 424:
        available = 1
        head = WorkItem(
            sequence_id=144,
            opcode=144,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551472,
        )
    if epoch >= 425 and epoch <= 426:
        available = 1
        head = WorkItem(
            sequence_id=145,
            opcode=161,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551471,
        )
    if epoch >= 427 and epoch <= 428:
        available = 1
        head = WorkItem(
            sequence_id=146,
            opcode=178,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551470,
        )
    if epoch >= 429 and epoch <= 430:
        available = 1
        head = WorkItem(
            sequence_id=147,
            opcode=195,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch >= 431 and epoch <= 432:
        available = 1
        head = WorkItem(
            sequence_id=148,
            opcode=212,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551468,
        )
    if epoch >= 433 and epoch <= 434:
        available = 1
        head = WorkItem(
            sequence_id=149,
            opcode=229,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551467,
        )
    if epoch >= 435 and epoch <= 436:
        available = 1
        head = WorkItem(
            sequence_id=150,
            opcode=246,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551466,
        )
    if epoch >= 437 and epoch <= 438:
        available = 1
        head = WorkItem(
            sequence_id=151,
            opcode=7,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch >= 439 and epoch <= 440:
        available = 1
        head = WorkItem(
            sequence_id=152,
            opcode=24,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551464,
        )
    if epoch >= 441 and epoch <= 442:
        available = 1
        head = WorkItem(
            sequence_id=153,
            opcode=41,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551463,
        )
    if epoch >= 443 and epoch <= 444:
        available = 1
        head = WorkItem(
            sequence_id=154,
            opcode=58,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551462,
        )
    if epoch >= 445 and epoch <= 446:
        available = 1
        head = WorkItem(
            sequence_id=155,
            opcode=75,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch >= 447 and epoch <= 448:
        available = 1
        head = WorkItem(
            sequence_id=156,
            opcode=92,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551460,
        )
    if epoch >= 449 and epoch <= 450:
        available = 1
        head = WorkItem(
            sequence_id=157,
            opcode=109,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551459,
        )
    if epoch >= 451 and epoch <= 452:
        available = 1
        head = WorkItem(
            sequence_id=158,
            opcode=126,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551458,
        )
    if epoch >= 453 and epoch <= 454:
        available = 1
        head = WorkItem(
            sequence_id=159,
            opcode=143,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch >= 455 and epoch <= 456:
        available = 1
        head = WorkItem(
            sequence_id=160,
            opcode=160,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551456,
        )
    if epoch >= 457 and epoch <= 458:
        available = 1
        head = WorkItem(
            sequence_id=161,
            opcode=177,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551455,
        )
    if epoch >= 459 and epoch <= 460:
        available = 1
        head = WorkItem(
            sequence_id=162,
            opcode=194,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551454,
        )
    if epoch >= 461 and epoch <= 462:
        available = 1
        head = WorkItem(
            sequence_id=163,
            opcode=211,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch >= 463 and epoch <= 464:
        available = 1
        head = WorkItem(
            sequence_id=164,
            opcode=228,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551452,
        )
    if epoch >= 465 and epoch <= 466:
        available = 1
        head = WorkItem(
            sequence_id=165,
            opcode=245,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551451,
        )
    if epoch >= 467 and epoch <= 468:
        available = 1
        head = WorkItem(
            sequence_id=166,
            opcode=6,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551450,
        )
    if epoch >= 469 and epoch <= 470:
        available = 1
        head = WorkItem(
            sequence_id=167,
            opcode=23,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch >= 471 and epoch <= 472:
        available = 1
        head = WorkItem(
            sequence_id=168,
            opcode=40,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551448,
        )
    if epoch >= 473 and epoch <= 474:
        available = 1
        head = WorkItem(
            sequence_id=169,
            opcode=57,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551447,
        )
    if epoch >= 475 and epoch <= 476:
        available = 1
        head = WorkItem(
            sequence_id=170,
            opcode=74,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551446,
        )
    if epoch >= 477 and epoch <= 478:
        available = 1
        head = WorkItem(
            sequence_id=171,
            opcode=91,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch >= 479 and epoch <= 480:
        available = 1
        head = WorkItem(
            sequence_id=172,
            opcode=108,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551444,
        )
    if epoch >= 481 and epoch <= 482:
        available = 1
        head = WorkItem(
            sequence_id=173,
            opcode=125,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551443,
        )
    if epoch >= 483 and epoch <= 484:
        available = 1
        head = WorkItem(
            sequence_id=174,
            opcode=142,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551442,
        )
    if epoch >= 485 and epoch <= 486:
        available = 1
        head = WorkItem(
            sequence_id=175,
            opcode=159,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    if epoch >= 487 and epoch <= 488:
        available = 1
        head = WorkItem(
            sequence_id=176,
            opcode=176,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551440,
        )
    if epoch >= 489 and epoch <= 490:
        available = 1
        head = WorkItem(
            sequence_id=177,
            opcode=193,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551439,
        )
    if epoch >= 491 and epoch <= 492:
        available = 1
        head = WorkItem(
            sequence_id=178,
            opcode=210,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551438,
        )
    if epoch >= 493 and epoch <= 494:
        available = 1
        head = WorkItem(
            sequence_id=179,
            opcode=227,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551437,
        )
    return ExpectedView(available=available, head=head)


@rule
def full_topology_backpressure_route_1_done(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 9:
        available = 1
        head = WorkItem(
            sequence_id=1, opcode=17, route=1, waits_for=255, cycles=1, value=1
        )
    if epoch == 13:
        available = 1
        head = WorkItem(
            sequence_id=5,
            opcode=85,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 17:
        available = 1
        head = WorkItem(
            sequence_id=9,
            opcode=153,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 21:
        available = 1
        head = WorkItem(
            sequence_id=13,
            opcode=221,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 25:
        available = 1
        head = WorkItem(
            sequence_id=17,
            opcode=33,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 29:
        available = 1
        head = WorkItem(
            sequence_id=21,
            opcode=101,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 33:
        available = 1
        head = WorkItem(
            sequence_id=25,
            opcode=169,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 37:
        available = 1
        head = WorkItem(
            sequence_id=29,
            opcode=237,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 41:
        available = 1
        head = WorkItem(
            sequence_id=33,
            opcode=49,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 45:
        available = 1
        head = WorkItem(
            sequence_id=37,
            opcode=117,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 49:
        available = 1
        head = WorkItem(
            sequence_id=41,
            opcode=185,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 53:
        available = 1
        head = WorkItem(
            sequence_id=45,
            opcode=253,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 57:
        available = 1
        head = WorkItem(
            sequence_id=49,
            opcode=65,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 61:
        available = 1
        head = WorkItem(
            sequence_id=53,
            opcode=133,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 65:
        available = 1
        head = WorkItem(
            sequence_id=57,
            opcode=201,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 69:
        available = 1
        head = WorkItem(
            sequence_id=61,
            opcode=13,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 73:
        available = 1
        head = WorkItem(
            sequence_id=65,
            opcode=81,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 77:
        available = 1
        head = WorkItem(
            sequence_id=69,
            opcode=149,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 81:
        available = 1
        head = WorkItem(
            sequence_id=73,
            opcode=217,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 85:
        available = 1
        head = WorkItem(
            sequence_id=77,
            opcode=29,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch >= 89 and epoch <= 368:
        available = 1
        head = WorkItem(
            sequence_id=81,
            opcode=97,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch >= 370 and epoch <= 376:
        available = 1
        head = WorkItem(
            sequence_id=85,
            opcode=165,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch >= 378 and epoch <= 384:
        available = 1
        head = WorkItem(
            sequence_id=89,
            opcode=233,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch >= 386 and epoch <= 392:
        available = 1
        head = WorkItem(
            sequence_id=93,
            opcode=45,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch >= 394 and epoch <= 400:
        available = 1
        head = WorkItem(
            sequence_id=97,
            opcode=113,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch >= 402 and epoch <= 408:
        available = 1
        head = WorkItem(
            sequence_id=101,
            opcode=181,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch >= 410 and epoch <= 416:
        available = 1
        head = WorkItem(
            sequence_id=105,
            opcode=249,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch >= 418 and epoch <= 424:
        available = 1
        head = WorkItem(
            sequence_id=109,
            opcode=61,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch >= 426 and epoch <= 432:
        available = 1
        head = WorkItem(
            sequence_id=113,
            opcode=129,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch >= 434 and epoch <= 440:
        available = 1
        head = WorkItem(
            sequence_id=117,
            opcode=197,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch >= 442 and epoch <= 448:
        available = 1
        head = WorkItem(
            sequence_id=121,
            opcode=9,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch >= 450 and epoch <= 456:
        available = 1
        head = WorkItem(
            sequence_id=125,
            opcode=77,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch >= 458 and epoch <= 464:
        available = 1
        head = WorkItem(
            sequence_id=129,
            opcode=145,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch >= 466 and epoch <= 472:
        available = 1
        head = WorkItem(
            sequence_id=133,
            opcode=213,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch >= 474 and epoch <= 480:
        available = 1
        head = WorkItem(
            sequence_id=137,
            opcode=25,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch >= 482 and epoch <= 488:
        available = 1
        head = WorkItem(
            sequence_id=141,
            opcode=93,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch >= 490 and epoch <= 496:
        available = 1
        head = WorkItem(
            sequence_id=145,
            opcode=161,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch >= 498 and epoch <= 504:
        available = 1
        head = WorkItem(
            sequence_id=149,
            opcode=229,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch >= 506 and epoch <= 512:
        available = 1
        head = WorkItem(
            sequence_id=153,
            opcode=41,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch >= 514 and epoch <= 520:
        available = 1
        head = WorkItem(
            sequence_id=157,
            opcode=109,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch >= 522 and epoch <= 528:
        available = 1
        head = WorkItem(
            sequence_id=161,
            opcode=177,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch >= 530 and epoch <= 536:
        available = 1
        head = WorkItem(
            sequence_id=165,
            opcode=245,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch >= 538 and epoch <= 544:
        available = 1
        head = WorkItem(
            sequence_id=169,
            opcode=57,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch >= 546 and epoch <= 552:
        available = 1
        head = WorkItem(
            sequence_id=173,
            opcode=125,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch >= 554 and epoch <= 560:
        available = 1
        head = WorkItem(
            sequence_id=177,
            opcode=193,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    return ExpectedView(available=available, head=head)


@rule
def full_topology_backpressure_route_3_done(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 11:
        available = 1
        head = WorkItem(
            sequence_id=3, opcode=51, route=3, waits_for=255, cycles=1, value=1
        )
    if epoch == 15:
        available = 1
        head = WorkItem(
            sequence_id=7,
            opcode=119,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 19:
        available = 1
        head = WorkItem(
            sequence_id=11,
            opcode=187,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 23:
        available = 1
        head = WorkItem(
            sequence_id=15,
            opcode=255,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 27:
        available = 1
        head = WorkItem(
            sequence_id=19,
            opcode=67,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 31:
        available = 1
        head = WorkItem(
            sequence_id=23,
            opcode=135,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 35:
        available = 1
        head = WorkItem(
            sequence_id=27,
            opcode=203,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 39:
        available = 1
        head = WorkItem(
            sequence_id=31,
            opcode=15,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 43:
        available = 1
        head = WorkItem(
            sequence_id=35,
            opcode=83,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 47:
        available = 1
        head = WorkItem(
            sequence_id=39,
            opcode=151,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 51:
        available = 1
        head = WorkItem(
            sequence_id=43,
            opcode=219,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 55:
        available = 1
        head = WorkItem(
            sequence_id=47,
            opcode=31,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 59:
        available = 1
        head = WorkItem(
            sequence_id=51,
            opcode=99,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 63:
        available = 1
        head = WorkItem(
            sequence_id=55,
            opcode=167,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 67:
        available = 1
        head = WorkItem(
            sequence_id=59,
            opcode=235,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 71:
        available = 1
        head = WorkItem(
            sequence_id=63,
            opcode=47,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 75:
        available = 1
        head = WorkItem(
            sequence_id=67,
            opcode=115,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 79:
        available = 1
        head = WorkItem(
            sequence_id=71,
            opcode=183,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 83:
        available = 1
        head = WorkItem(
            sequence_id=75,
            opcode=251,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch == 87:
        available = 1
        head = WorkItem(
            sequence_id=79,
            opcode=63,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch >= 91 and epoch <= 372:
        available = 1
        head = WorkItem(
            sequence_id=83,
            opcode=131,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch >= 374 and epoch <= 380:
        available = 1
        head = WorkItem(
            sequence_id=87,
            opcode=199,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch >= 382 and epoch <= 388:
        available = 1
        head = WorkItem(
            sequence_id=91,
            opcode=11,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch >= 390 and epoch <= 396:
        available = 1
        head = WorkItem(
            sequence_id=95,
            opcode=79,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch >= 398 and epoch <= 404:
        available = 1
        head = WorkItem(
            sequence_id=99,
            opcode=147,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch >= 406 and epoch <= 412:
        available = 1
        head = WorkItem(
            sequence_id=103,
            opcode=215,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch >= 414 and epoch <= 420:
        available = 1
        head = WorkItem(
            sequence_id=107,
            opcode=27,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch >= 422 and epoch <= 428:
        available = 1
        head = WorkItem(
            sequence_id=111,
            opcode=95,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch >= 430 and epoch <= 436:
        available = 1
        head = WorkItem(
            sequence_id=115,
            opcode=163,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch >= 438 and epoch <= 444:
        available = 1
        head = WorkItem(
            sequence_id=119,
            opcode=231,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch >= 446 and epoch <= 452:
        available = 1
        head = WorkItem(
            sequence_id=123,
            opcode=43,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch >= 454 and epoch <= 460:
        available = 1
        head = WorkItem(
            sequence_id=127,
            opcode=111,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch >= 462 and epoch <= 468:
        available = 1
        head = WorkItem(
            sequence_id=131,
            opcode=179,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch >= 470 and epoch <= 476:
        available = 1
        head = WorkItem(
            sequence_id=135,
            opcode=247,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch >= 478 and epoch <= 484:
        available = 1
        head = WorkItem(
            sequence_id=139,
            opcode=59,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch >= 486 and epoch <= 492:
        available = 1
        head = WorkItem(
            sequence_id=143,
            opcode=127,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch >= 494 and epoch <= 500:
        available = 1
        head = WorkItem(
            sequence_id=147,
            opcode=195,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch >= 502 and epoch <= 508:
        available = 1
        head = WorkItem(
            sequence_id=151,
            opcode=7,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch >= 510 and epoch <= 516:
        available = 1
        head = WorkItem(
            sequence_id=155,
            opcode=75,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch >= 518 and epoch <= 524:
        available = 1
        head = WorkItem(
            sequence_id=159,
            opcode=143,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch >= 526 and epoch <= 532:
        available = 1
        head = WorkItem(
            sequence_id=163,
            opcode=211,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch >= 534 and epoch <= 540:
        available = 1
        head = WorkItem(
            sequence_id=167,
            opcode=23,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch >= 542 and epoch <= 548:
        available = 1
        head = WorkItem(
            sequence_id=171,
            opcode=91,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch >= 550 and epoch <= 556:
        available = 1
        head = WorkItem(
            sequence_id=175,
            opcode=159,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch >= 558 and epoch <= 564:
        available = 1
        head = WorkItem(
            sequence_id=179,
            opcode=227,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    return ExpectedView(available=available, head=head)


@rule
def full_topology_backpressure_completed(epoch: u16) -> ExpectedView:
    available: u1 = 0
    head = WorkItem(sequence_id=0, opcode=0, route=0, waits_for=0, cycles=0, value=0)
    if epoch == 9:
        available = 1
        head = WorkItem(
            sequence_id=0, opcode=0, route=0, waits_for=255, cycles=1, value=1
        )
    if epoch == 10:
        available = 1
        head = WorkItem(
            sequence_id=1, opcode=17, route=1, waits_for=255, cycles=1, value=1
        )
    if epoch == 11:
        available = 1
        head = WorkItem(
            sequence_id=2, opcode=34, route=2, waits_for=255, cycles=1, value=1
        )
    if epoch == 12:
        available = 1
        head = WorkItem(
            sequence_id=3, opcode=51, route=3, waits_for=255, cycles=1, value=1
        )
    if epoch == 13:
        available = 1
        head = WorkItem(
            sequence_id=4,
            opcode=68,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 14:
        available = 1
        head = WorkItem(
            sequence_id=5,
            opcode=85,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 15:
        available = 1
        head = WorkItem(
            sequence_id=6,
            opcode=102,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 16:
        available = 1
        head = WorkItem(
            sequence_id=7,
            opcode=119,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551613,
        )
    if epoch == 17:
        available = 1
        head = WorkItem(
            sequence_id=8,
            opcode=136,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 18:
        available = 1
        head = WorkItem(
            sequence_id=9,
            opcode=153,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 19:
        available = 1
        head = WorkItem(
            sequence_id=10,
            opcode=170,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 20:
        available = 1
        head = WorkItem(
            sequence_id=11,
            opcode=187,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551609,
        )
    if epoch == 21:
        available = 1
        head = WorkItem(
            sequence_id=12,
            opcode=204,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 22:
        available = 1
        head = WorkItem(
            sequence_id=13,
            opcode=221,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 23:
        available = 1
        head = WorkItem(
            sequence_id=14,
            opcode=238,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 24:
        available = 1
        head = WorkItem(
            sequence_id=15,
            opcode=255,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551605,
        )
    if epoch == 25:
        available = 1
        head = WorkItem(
            sequence_id=16,
            opcode=16,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 26:
        available = 1
        head = WorkItem(
            sequence_id=17,
            opcode=33,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 27:
        available = 1
        head = WorkItem(
            sequence_id=18,
            opcode=50,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 28:
        available = 1
        head = WorkItem(
            sequence_id=19,
            opcode=67,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551601,
        )
    if epoch == 29:
        available = 1
        head = WorkItem(
            sequence_id=20,
            opcode=84,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 30:
        available = 1
        head = WorkItem(
            sequence_id=21,
            opcode=101,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 31:
        available = 1
        head = WorkItem(
            sequence_id=22,
            opcode=118,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 32:
        available = 1
        head = WorkItem(
            sequence_id=23,
            opcode=135,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551597,
        )
    if epoch == 33:
        available = 1
        head = WorkItem(
            sequence_id=24,
            opcode=152,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 34:
        available = 1
        head = WorkItem(
            sequence_id=25,
            opcode=169,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 35:
        available = 1
        head = WorkItem(
            sequence_id=26,
            opcode=186,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 36:
        available = 1
        head = WorkItem(
            sequence_id=27,
            opcode=203,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551593,
        )
    if epoch == 37:
        available = 1
        head = WorkItem(
            sequence_id=28,
            opcode=220,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 38:
        available = 1
        head = WorkItem(
            sequence_id=29,
            opcode=237,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 39:
        available = 1
        head = WorkItem(
            sequence_id=30,
            opcode=254,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 40:
        available = 1
        head = WorkItem(
            sequence_id=31,
            opcode=15,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551589,
        )
    if epoch == 41:
        available = 1
        head = WorkItem(
            sequence_id=32,
            opcode=32,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 42:
        available = 1
        head = WorkItem(
            sequence_id=33,
            opcode=49,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 43:
        available = 1
        head = WorkItem(
            sequence_id=34,
            opcode=66,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 44:
        available = 1
        head = WorkItem(
            sequence_id=35,
            opcode=83,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551585,
        )
    if epoch == 45:
        available = 1
        head = WorkItem(
            sequence_id=36,
            opcode=100,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 46:
        available = 1
        head = WorkItem(
            sequence_id=37,
            opcode=117,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 47:
        available = 1
        head = WorkItem(
            sequence_id=38,
            opcode=134,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 48:
        available = 1
        head = WorkItem(
            sequence_id=39,
            opcode=151,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551581,
        )
    if epoch == 49:
        available = 1
        head = WorkItem(
            sequence_id=40,
            opcode=168,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 50:
        available = 1
        head = WorkItem(
            sequence_id=41,
            opcode=185,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 51:
        available = 1
        head = WorkItem(
            sequence_id=42,
            opcode=202,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 52:
        available = 1
        head = WorkItem(
            sequence_id=43,
            opcode=219,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551577,
        )
    if epoch == 53:
        available = 1
        head = WorkItem(
            sequence_id=44,
            opcode=236,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 54:
        available = 1
        head = WorkItem(
            sequence_id=45,
            opcode=253,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 55:
        available = 1
        head = WorkItem(
            sequence_id=46,
            opcode=14,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 56:
        available = 1
        head = WorkItem(
            sequence_id=47,
            opcode=31,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551573,
        )
    if epoch == 57:
        available = 1
        head = WorkItem(
            sequence_id=48,
            opcode=48,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 58:
        available = 1
        head = WorkItem(
            sequence_id=49,
            opcode=65,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 59:
        available = 1
        head = WorkItem(
            sequence_id=50,
            opcode=82,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 60:
        available = 1
        head = WorkItem(
            sequence_id=51,
            opcode=99,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551569,
        )
    if epoch == 61:
        available = 1
        head = WorkItem(
            sequence_id=52,
            opcode=116,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 62:
        available = 1
        head = WorkItem(
            sequence_id=53,
            opcode=133,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 63:
        available = 1
        head = WorkItem(
            sequence_id=54,
            opcode=150,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 64:
        available = 1
        head = WorkItem(
            sequence_id=55,
            opcode=167,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551565,
        )
    if epoch == 65:
        available = 1
        head = WorkItem(
            sequence_id=56,
            opcode=184,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 66:
        available = 1
        head = WorkItem(
            sequence_id=57,
            opcode=201,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 67:
        available = 1
        head = WorkItem(
            sequence_id=58,
            opcode=218,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 68:
        available = 1
        head = WorkItem(
            sequence_id=59,
            opcode=235,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551561,
        )
    if epoch == 69:
        available = 1
        head = WorkItem(
            sequence_id=60,
            opcode=252,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 70:
        available = 1
        head = WorkItem(
            sequence_id=61,
            opcode=13,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 71:
        available = 1
        head = WorkItem(
            sequence_id=62,
            opcode=30,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 72:
        available = 1
        head = WorkItem(
            sequence_id=63,
            opcode=47,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551557,
        )
    if epoch == 73:
        available = 1
        head = WorkItem(
            sequence_id=64,
            opcode=64,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 74:
        available = 1
        head = WorkItem(
            sequence_id=65,
            opcode=81,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 75:
        available = 1
        head = WorkItem(
            sequence_id=66,
            opcode=98,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 76:
        available = 1
        head = WorkItem(
            sequence_id=67,
            opcode=115,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551553,
        )
    if epoch == 77:
        available = 1
        head = WorkItem(
            sequence_id=68,
            opcode=132,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 78:
        available = 1
        head = WorkItem(
            sequence_id=69,
            opcode=149,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 79:
        available = 1
        head = WorkItem(
            sequence_id=70,
            opcode=166,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 80:
        available = 1
        head = WorkItem(
            sequence_id=71,
            opcode=183,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551549,
        )
    if epoch == 81:
        available = 1
        head = WorkItem(
            sequence_id=72,
            opcode=200,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch >= 82 and epoch <= 367:
        available = 1
        head = WorkItem(
            sequence_id=73,
            opcode=217,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch >= 368 and epoch <= 369:
        available = 1
        head = WorkItem(
            sequence_id=74,
            opcode=234,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch >= 370 and epoch <= 371:
        available = 1
        head = WorkItem(
            sequence_id=75,
            opcode=251,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551545,
        )
    if epoch >= 372 and epoch <= 373:
        available = 1
        head = WorkItem(
            sequence_id=76,
            opcode=12,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch >= 374 and epoch <= 375:
        available = 1
        head = WorkItem(
            sequence_id=77,
            opcode=29,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch >= 376 and epoch <= 377:
        available = 1
        head = WorkItem(
            sequence_id=78,
            opcode=46,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch >= 378 and epoch <= 379:
        available = 1
        head = WorkItem(
            sequence_id=79,
            opcode=63,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551541,
        )
    if epoch >= 380 and epoch <= 381:
        available = 1
        head = WorkItem(
            sequence_id=80,
            opcode=80,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch >= 382 and epoch <= 383:
        available = 1
        head = WorkItem(
            sequence_id=81,
            opcode=97,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch >= 384 and epoch <= 385:
        available = 1
        head = WorkItem(
            sequence_id=82,
            opcode=114,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch >= 386 and epoch <= 387:
        available = 1
        head = WorkItem(
            sequence_id=83,
            opcode=131,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551537,
        )
    if epoch >= 388 and epoch <= 389:
        available = 1
        head = WorkItem(
            sequence_id=84,
            opcode=148,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch >= 390 and epoch <= 391:
        available = 1
        head = WorkItem(
            sequence_id=85,
            opcode=165,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch >= 392 and epoch <= 393:
        available = 1
        head = WorkItem(
            sequence_id=86,
            opcode=182,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch >= 394 and epoch <= 395:
        available = 1
        head = WorkItem(
            sequence_id=87,
            opcode=199,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551533,
        )
    if epoch >= 396 and epoch <= 397:
        available = 1
        head = WorkItem(
            sequence_id=88,
            opcode=216,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch >= 398 and epoch <= 399:
        available = 1
        head = WorkItem(
            sequence_id=89,
            opcode=233,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch >= 400 and epoch <= 401:
        available = 1
        head = WorkItem(
            sequence_id=90,
            opcode=250,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch >= 402 and epoch <= 403:
        available = 1
        head = WorkItem(
            sequence_id=91,
            opcode=11,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551529,
        )
    if epoch >= 404 and epoch <= 405:
        available = 1
        head = WorkItem(
            sequence_id=92,
            opcode=28,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch >= 406 and epoch <= 407:
        available = 1
        head = WorkItem(
            sequence_id=93,
            opcode=45,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch >= 408 and epoch <= 409:
        available = 1
        head = WorkItem(
            sequence_id=94,
            opcode=62,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch >= 410 and epoch <= 411:
        available = 1
        head = WorkItem(
            sequence_id=95,
            opcode=79,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551525,
        )
    if epoch >= 412 and epoch <= 413:
        available = 1
        head = WorkItem(
            sequence_id=96,
            opcode=96,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch >= 414 and epoch <= 415:
        available = 1
        head = WorkItem(
            sequence_id=97,
            opcode=113,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch >= 416 and epoch <= 417:
        available = 1
        head = WorkItem(
            sequence_id=98,
            opcode=130,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch >= 418 and epoch <= 419:
        available = 1
        head = WorkItem(
            sequence_id=99,
            opcode=147,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551521,
        )
    if epoch >= 420 and epoch <= 421:
        available = 1
        head = WorkItem(
            sequence_id=100,
            opcode=164,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch >= 422 and epoch <= 423:
        available = 1
        head = WorkItem(
            sequence_id=101,
            opcode=181,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch >= 424 and epoch <= 425:
        available = 1
        head = WorkItem(
            sequence_id=102,
            opcode=198,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch >= 426 and epoch <= 427:
        available = 1
        head = WorkItem(
            sequence_id=103,
            opcode=215,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551517,
        )
    if epoch >= 428 and epoch <= 429:
        available = 1
        head = WorkItem(
            sequence_id=104,
            opcode=232,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch >= 430 and epoch <= 431:
        available = 1
        head = WorkItem(
            sequence_id=105,
            opcode=249,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch >= 432 and epoch <= 433:
        available = 1
        head = WorkItem(
            sequence_id=106,
            opcode=10,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch >= 434 and epoch <= 435:
        available = 1
        head = WorkItem(
            sequence_id=107,
            opcode=27,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551513,
        )
    if epoch >= 436 and epoch <= 437:
        available = 1
        head = WorkItem(
            sequence_id=108,
            opcode=44,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch >= 438 and epoch <= 439:
        available = 1
        head = WorkItem(
            sequence_id=109,
            opcode=61,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch >= 440 and epoch <= 441:
        available = 1
        head = WorkItem(
            sequence_id=110,
            opcode=78,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch >= 442 and epoch <= 443:
        available = 1
        head = WorkItem(
            sequence_id=111,
            opcode=95,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551509,
        )
    if epoch >= 444 and epoch <= 445:
        available = 1
        head = WorkItem(
            sequence_id=112,
            opcode=112,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch >= 446 and epoch <= 447:
        available = 1
        head = WorkItem(
            sequence_id=113,
            opcode=129,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch >= 448 and epoch <= 449:
        available = 1
        head = WorkItem(
            sequence_id=114,
            opcode=146,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch >= 450 and epoch <= 451:
        available = 1
        head = WorkItem(
            sequence_id=115,
            opcode=163,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551505,
        )
    if epoch >= 452 and epoch <= 453:
        available = 1
        head = WorkItem(
            sequence_id=116,
            opcode=180,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch >= 454 and epoch <= 455:
        available = 1
        head = WorkItem(
            sequence_id=117,
            opcode=197,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch >= 456 and epoch <= 457:
        available = 1
        head = WorkItem(
            sequence_id=118,
            opcode=214,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch >= 458 and epoch <= 459:
        available = 1
        head = WorkItem(
            sequence_id=119,
            opcode=231,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551501,
        )
    if epoch >= 460 and epoch <= 461:
        available = 1
        head = WorkItem(
            sequence_id=120,
            opcode=248,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch >= 462 and epoch <= 463:
        available = 1
        head = WorkItem(
            sequence_id=121,
            opcode=9,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch >= 464 and epoch <= 465:
        available = 1
        head = WorkItem(
            sequence_id=122,
            opcode=26,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch >= 466 and epoch <= 467:
        available = 1
        head = WorkItem(
            sequence_id=123,
            opcode=43,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551497,
        )
    if epoch >= 468 and epoch <= 469:
        available = 1
        head = WorkItem(
            sequence_id=124,
            opcode=60,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch >= 470 and epoch <= 471:
        available = 1
        head = WorkItem(
            sequence_id=125,
            opcode=77,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch >= 472 and epoch <= 473:
        available = 1
        head = WorkItem(
            sequence_id=126,
            opcode=94,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch >= 474 and epoch <= 475:
        available = 1
        head = WorkItem(
            sequence_id=127,
            opcode=111,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551493,
        )
    if epoch >= 476 and epoch <= 477:
        available = 1
        head = WorkItem(
            sequence_id=128,
            opcode=128,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch >= 478 and epoch <= 479:
        available = 1
        head = WorkItem(
            sequence_id=129,
            opcode=145,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch >= 480 and epoch <= 481:
        available = 1
        head = WorkItem(
            sequence_id=130,
            opcode=162,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch >= 482 and epoch <= 483:
        available = 1
        head = WorkItem(
            sequence_id=131,
            opcode=179,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551489,
        )
    if epoch >= 484 and epoch <= 485:
        available = 1
        head = WorkItem(
            sequence_id=132,
            opcode=196,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch >= 486 and epoch <= 487:
        available = 1
        head = WorkItem(
            sequence_id=133,
            opcode=213,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch >= 488 and epoch <= 489:
        available = 1
        head = WorkItem(
            sequence_id=134,
            opcode=230,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch >= 490 and epoch <= 491:
        available = 1
        head = WorkItem(
            sequence_id=135,
            opcode=247,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551485,
        )
    if epoch >= 492 and epoch <= 493:
        available = 1
        head = WorkItem(
            sequence_id=136,
            opcode=8,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch >= 494 and epoch <= 495:
        available = 1
        head = WorkItem(
            sequence_id=137,
            opcode=25,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch >= 496 and epoch <= 497:
        available = 1
        head = WorkItem(
            sequence_id=138,
            opcode=42,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch >= 498 and epoch <= 499:
        available = 1
        head = WorkItem(
            sequence_id=139,
            opcode=59,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551481,
        )
    if epoch >= 500 and epoch <= 501:
        available = 1
        head = WorkItem(
            sequence_id=140,
            opcode=76,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch >= 502 and epoch <= 503:
        available = 1
        head = WorkItem(
            sequence_id=141,
            opcode=93,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch >= 504 and epoch <= 505:
        available = 1
        head = WorkItem(
            sequence_id=142,
            opcode=110,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch >= 506 and epoch <= 507:
        available = 1
        head = WorkItem(
            sequence_id=143,
            opcode=127,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551477,
        )
    if epoch >= 508 and epoch <= 509:
        available = 1
        head = WorkItem(
            sequence_id=144,
            opcode=144,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch >= 510 and epoch <= 511:
        available = 1
        head = WorkItem(
            sequence_id=145,
            opcode=161,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch >= 512 and epoch <= 513:
        available = 1
        head = WorkItem(
            sequence_id=146,
            opcode=178,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch >= 514 and epoch <= 515:
        available = 1
        head = WorkItem(
            sequence_id=147,
            opcode=195,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551473,
        )
    if epoch >= 516 and epoch <= 517:
        available = 1
        head = WorkItem(
            sequence_id=148,
            opcode=212,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch >= 518 and epoch <= 519:
        available = 1
        head = WorkItem(
            sequence_id=149,
            opcode=229,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch >= 520 and epoch <= 521:
        available = 1
        head = WorkItem(
            sequence_id=150,
            opcode=246,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch >= 522 and epoch <= 523:
        available = 1
        head = WorkItem(
            sequence_id=151,
            opcode=7,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551469,
        )
    if epoch >= 524 and epoch <= 525:
        available = 1
        head = WorkItem(
            sequence_id=152,
            opcode=24,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch >= 526 and epoch <= 527:
        available = 1
        head = WorkItem(
            sequence_id=153,
            opcode=41,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch >= 528 and epoch <= 529:
        available = 1
        head = WorkItem(
            sequence_id=154,
            opcode=58,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch >= 530 and epoch <= 531:
        available = 1
        head = WorkItem(
            sequence_id=155,
            opcode=75,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551465,
        )
    if epoch >= 532 and epoch <= 533:
        available = 1
        head = WorkItem(
            sequence_id=156,
            opcode=92,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch >= 534 and epoch <= 535:
        available = 1
        head = WorkItem(
            sequence_id=157,
            opcode=109,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch >= 536 and epoch <= 537:
        available = 1
        head = WorkItem(
            sequence_id=158,
            opcode=126,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch >= 538 and epoch <= 539:
        available = 1
        head = WorkItem(
            sequence_id=159,
            opcode=143,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551461,
        )
    if epoch >= 540 and epoch <= 541:
        available = 1
        head = WorkItem(
            sequence_id=160,
            opcode=160,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch >= 542 and epoch <= 543:
        available = 1
        head = WorkItem(
            sequence_id=161,
            opcode=177,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch >= 544 and epoch <= 545:
        available = 1
        head = WorkItem(
            sequence_id=162,
            opcode=194,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch >= 546 and epoch <= 547:
        available = 1
        head = WorkItem(
            sequence_id=163,
            opcode=211,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551457,
        )
    if epoch >= 548 and epoch <= 549:
        available = 1
        head = WorkItem(
            sequence_id=164,
            opcode=228,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch >= 550 and epoch <= 551:
        available = 1
        head = WorkItem(
            sequence_id=165,
            opcode=245,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch >= 552 and epoch <= 553:
        available = 1
        head = WorkItem(
            sequence_id=166,
            opcode=6,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch >= 554 and epoch <= 555:
        available = 1
        head = WorkItem(
            sequence_id=167,
            opcode=23,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551453,
        )
    if epoch >= 556 and epoch <= 557:
        available = 1
        head = WorkItem(
            sequence_id=168,
            opcode=40,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch >= 558 and epoch <= 559:
        available = 1
        head = WorkItem(
            sequence_id=169,
            opcode=57,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch >= 560 and epoch <= 561:
        available = 1
        head = WorkItem(
            sequence_id=170,
            opcode=74,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch >= 562 and epoch <= 563:
        available = 1
        head = WorkItem(
            sequence_id=171,
            opcode=91,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551449,
        )
    if epoch >= 564 and epoch <= 565:
        available = 1
        head = WorkItem(
            sequence_id=172,
            opcode=108,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch >= 566 and epoch <= 567:
        available = 1
        head = WorkItem(
            sequence_id=173,
            opcode=125,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch >= 568 and epoch <= 569:
        available = 1
        head = WorkItem(
            sequence_id=174,
            opcode=142,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch >= 570 and epoch <= 571:
        available = 1
        head = WorkItem(
            sequence_id=175,
            opcode=159,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551445,
        )
    if epoch >= 572 and epoch <= 573:
        available = 1
        head = WorkItem(
            sequence_id=176,
            opcode=176,
            route=0,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    if epoch >= 574 and epoch <= 575:
        available = 1
        head = WorkItem(
            sequence_id=177,
            opcode=193,
            route=1,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    if epoch >= 576 and epoch <= 577:
        available = 1
        head = WorkItem(
            sequence_id=178,
            opcode=210,
            route=2,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    if epoch >= 578 and epoch <= 579:
        available = 1
        head = WorkItem(
            sequence_id=179,
            opcode=227,
            route=3,
            waits_for=255,
            cycles=1,
            value=18446744073709551441,
        )
    return ExpectedView(available=available, head=head)


@system
def routed_full_topology_backpressure():
    epoch: u16 = 0
    inputs = full_topology_backpressure_stimulus(epoch)
    result = pipeline(inputs.valid, inputs.data, inputs.take)
    expected_ready = full_topology_backpressure_ready(epoch)
    expected_output = full_topology_backpressure_output(epoch)
    expected_scheduled = full_topology_backpressure_scheduled(epoch)
    expected_route_1_done = full_topology_backpressure_route_1_done(epoch)
    expected_route_3_done = full_topology_backpressure_route_3_done(epoch)
    expected_completed = full_topology_backpressure_completed(epoch)
    observe(
        epoch,
        result,
        expected_ready,
        expected_output,
        expected_scheduled,
        expected_route_1_done,
        expected_route_3_done,
        expected_completed,
    )
