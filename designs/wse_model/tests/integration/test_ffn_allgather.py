"""End-to-end FFN two-phase AllGather over the Calendar/NoC closure."""

from __future__ import annotations

import pytest

from wse_model.calendar.receive import ReceiveAccount
from wse_model.calendar.route_bits import RouteBits
from wse_model.collective import run_allgather
from wse_model.errors import CalendarError
from wse_model.fixtures import ffn_example
from wse_model.noc import CalRegImage, CalRegSlot, Noc, SendPlan, simulate_multicast

pytestmark = pytest.mark.integration

OPCODE_ALLGATHER = 1


def _noc(example, *, opcodes=(1,)) -> Noc:
    return Noc(
        example.table.topology,
        calreg=CalRegImage(
            slots=tuple(
                CalRegSlot(opcode=opcode, content=bytes([opcode]), arm_lead_cycles=64)
                for opcode in opcodes
            ),
            calendar_version=1,
        ),
    )


def test_ffn_two_phase_allgather_completes_on_every_member() -> None:
    example = ffn_example()
    noc = _noc(example)

    phase_b = run_allgather(
        noc,
        example.table,
        0,
        geometry=example.phase_b_geometry,
        groups=example.phase_b_groups,
    )
    phase_b.check()
    assert len(phase_b.members) == 32
    assert all(state.complete for state in phase_b.states)
    assert {state.exp_val for state in phase_b.states} == {10752}
    assert all(state.land_count == 7 for state in phase_b.states)

    phase_c = run_allgather(
        noc,
        example.table,
        1,
        geometry=example.phase_c_geometry,
        groups=example.phase_c_groups,
    )
    phase_c.check()
    assert {state.exp_val for state in phase_c.states} == {36864}
    assert all(state.land_count == 3 for state in phase_c.states)


def test_shared_opcode_domain_advances_the_epoch_across_phases() -> None:
    """Both FFN phases use opcode 1, so the round numbers are 1 and 2."""
    example = ffn_example()
    noc = _noc(example)
    first = run_allgather(
        noc, example.table, 0, geometry=example.phase_b_geometry, groups=example.phase_b_groups
    )
    second = run_allgather(
        noc, example.table, 1, geometry=example.phase_c_geometry, groups=example.phase_c_groups
    )
    assert (first.epoch, second.epoch) == (1, 2)
    assert first.opcode == second.opcode == OPCODE_ALLGATHER


def test_every_member_of_a_row_group_receives_from_the_other_seven() -> None:
    example = ffn_example()
    noc = _noc(example)
    report = run_allgather(
        noc, example.table, 0, geometry=example.phase_b_geometry, groups=example.phase_b_groups
    )
    for state in report.states:
        assert state.counted_bytes == state.exp_val
        # 8 rows x 192 B x 7 remote sources.
        assert state.counted_bytes == 8 * 192 * 7


def test_the_phase_reports_the_header_cost_and_tree_depth() -> None:
    example = ffn_example()
    noc = _noc(example)
    report = run_allgather(
        noc, example.table, 0, geometry=example.phase_b_geometry, groups=example.phase_b_groups
    )
    # A unidirectional row walk from an edge cell is 7 hops deep.
    assert report.max_tree_depth == 7
    assert report.peak_link_load > 0
    assert 0 < report.total_header_bytes < report.total_wire_bytes
    # Payload is what reaches the destinations; headers are pure overhead.
    assert report.total_payload_bytes < report.total_wire_bytes


def test_launch_boundary_drain_is_observable() -> None:
    example = ffn_example()
    noc = _noc(example)
    run_allgather(
        noc, example.table, 0, geometry=example.phase_b_geometry, groups=example.phase_b_groups
    )
    # Every transfer was drained in Step5, so the die is idle again.
    assert noc.drained


def test_running_a_phase_without_installed_calreg_faults() -> None:
    example = ffn_example()
    noc = Noc(example.table.topology)  # no CalReg image installed
    with pytest.raises(CalendarError) as excinfo:
        run_allgather(
            noc,
            example.table,
            0,
            geometry=example.phase_b_geometry,
            groups=example.phase_b_groups,
        )
    assert "CalReg" in str(excinfo.value)


def test_a_missing_receive_command_is_detected_at_join() -> None:
    """Step5 must not report success when a destination never posted Step3."""
    example = ffn_example()
    noc = _noc(example)
    member = 0
    account = ReceiveAccount(
        aicore=member, opcode=1, epoch=1, dst=0x1000, capacity=20000, exp_val=10752
    )
    # Post for the member itself only; its peers have no receive command, so the
    # bytes they should have counted land in their ingress ledgers as pending.
    noc.ingress[member].post(account)
    plan = SendPlan(
        source=1,
        route_bits=example.table.entry(0, 1).route_bits,
        opcode=1,
        epoch=1,
        row_count=8,
        row_bytes=192,
        arena_base=0x1000,
        row_stride=example.phase_b_geometry.recv_row_stride,
        source_self_off=192,
    )
    noc.send(plan)
    # The peer's arrivals are buffered, not lost: contract 4 keeps them countable.
    assert noc.ingress[2].pending.get((1, 1))


def test_wrong_epoch_is_reported_as_a_contract_violation() -> None:
    example = ffn_example()
    noc = _noc(example)
    member = 0
    noc.ingress[member].post(
        ReceiveAccount(aicore=member, opcode=1, epoch=5, dst=0x1000, capacity=20000, exp_val=10752)
    )
    plan = SendPlan(
        source=1,
        route_bits=example.table.entry(0, 1).route_bits,
        opcode=1,
        epoch=1,
        row_count=1,
        row_bytes=192,
        arena_base=0x1000,
        row_stride=example.phase_b_geometry.recv_row_stride,
        source_self_off=192,
    )
    # The delivery is buffered under epoch 1, so the epoch-5 command stays
    # incomplete rather than silently accepting it.
    noc.send(plan)
    assert noc.ingress[member].outstanding[0].epoch == 5
    assert not noc.ingress[member].outstanding[0].complete


def test_collective_requires_installed_calreg_before_any_send() -> None:
    example = ffn_example()
    noc = Noc(example.table.topology)
    with pytest.raises(CalendarError):
        noc.send(
            SendPlan(
                source=0,
                route_bits=example.table.entry(0, 0).route_bits,
                opcode=1,
                epoch=1,
                row_count=8,
                row_bytes=192,
                arena_base=0x1000,
                row_stride=1536,
                source_self_off=0,
            )
        )


def test_the_closure_refuses_to_model_reduce() -> None:
    """Reduce is blocked on open item C-10, so the closure must not fake it."""
    from wse_model.calendar.collective import Collective
    from wse_model.calendar.entry import CalendarRouteEntry, RouteEntryFlags
    from wse_model.calendar.key import GroupRole, RouteKey
    from wse_model.calendar.table import KeyDefinition, build_table
    from wse_model.topology import CALENDAR_BASELINE

    topology = CALENDAR_BASELINE.topology
    group = (0, 1)
    rows = tuple(
        CalendarRouteEntry(
            route_bits=RouteBits.from_sets(
                topology.node_count,
                pass_nodes={node},
                land_nodes=set(),
            ),
            flags=RouteEntryFlags.IS_MEMBER,
        )
        for node in topology.nodes
    )
    table = build_table(
        topology=topology,
        definitions=(
            KeyDefinition(
                key_id=0,
                route_key=RouteKey(
                    program_id="p",
                    kernel_id="k",
                    phase_id="reduce",
                    collective=Collective.REDUCE,
                    group_role=GroupRole(kind="ROW", axis=0),
                ),
                rows=rows,
                member_nodes=frozenset(group),
                groups=dict.fromkeys(group, group),
            ),
        ),
    )
    noc = _noc(ffn_example())
    with pytest.raises(CalendarError) as excinfo:
        run_allgather(noc, table, 0, geometry=ffn_example().phase_b_geometry, groups={0: group})
    assert "C-10" in str(excinfo.value)


def test_the_ffn_row_bitmaps_are_valid_trees() -> None:
    """Every member source's bitmap must pass the induced-tree check."""
    example = ffn_example()
    for key_id in (0, 1):
        definition = example.table.key(key_id)
        for source in sorted(definition.member_nodes):
            trace = simulate_multicast(
                example.table.topology,
                definition.entry(source).route_bits,
                source=source,
            )
            assert trace.is_clean_tree_walk
            assert set(trace.land_nodes) == set(definition.group_of(source)) - {source}
