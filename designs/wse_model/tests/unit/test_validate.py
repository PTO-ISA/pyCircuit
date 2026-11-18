"""Structural validation: the §2.7.2 check set."""

from __future__ import annotations

import pytest

from wse_model.calendar.collective import Collective
from wse_model.calendar.entry import CalendarRouteEntry
from wse_model.calendar.geometry import PayloadGeometry
from wse_model.calendar.key import GroupRole, RouteKey
from wse_model.calendar.route_bits import RouteBits
from wse_model.calendar.table import (
    KeyDefinition,
    TableLayout,
    build_table,
)
from wse_model.calendar.validate import (
    check_induced_tree,
    validate_entry_layout,
    validate_route_bits,
    validate_table_budget,
)
from wse_model.errors import CalendarError
from wse_model.topology import CALENDAR_BASELINE

pytestmark = pytest.mark.unit

TOPOLOGY = CALENDAR_BASELINE.topology
NODES = TOPOLOGY.node_count


def _route_key(phase: str = "p") -> RouteKey:
    return RouteKey(
        program_id="prog",
        kernel_id="kern",
        phase_id=phase,
        collective=Collective.ALL_GATHER,
        group_role=GroupRole(kind="ROW", axis="my_row"),
    )


# -- bit pairs -------------------------------------------------------------


def test_illegal_bit_pair_is_reported_with_its_node() -> None:
    bitmap = RouteBits.from_int(0b01 << (2 * 5), NODES)
    report = validate_route_bits(TOPOLOGY, bitmap, key_id=0)
    assert not report.ok
    assert [d.code for d in report.errors] == ["V-PAIR"]
    assert report.errors[0].node == 5


def test_source_must_have_p_set() -> None:
    bitmap = RouteBits.from_sets(NODES, pass_nodes={1, 2}, land_nodes={2})
    report = validate_route_bits(TOPOLOGY, bitmap, key_id=0, source=0)
    codes = {d.code for d in report.errors}
    assert "V-PSELF" in codes


# -- induced tree ----------------------------------------------------------


def test_a_simple_path_is_a_tree() -> None:
    # N00 -> N01 -> N02 -> N03, landing at N02 and N03.
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 2, 3}, land_nodes={2, 3})
    analysis = check_induced_tree(TOPOLOGY, bitmap, source=0)
    assert analysis.is_tree
    assert analysis.reached == frozenset({0, 1, 2, 3})


def test_a_two_by_two_block_is_rejected_as_cyclic() -> None:
    """Calendar §2.2's own warning case: adjacent P nodes create an unintended cycle."""
    block = {0, 1, 8, 9}
    bitmap = RouteBits.from_sets(NODES, pass_nodes=block, land_nodes=block)
    analysis = check_induced_tree(TOPOLOGY, bitmap, source=0)
    assert analysis.is_connected
    assert not analysis.is_acyclic
    assert not analysis.is_tree
    report = validate_route_bits(TOPOLOGY, bitmap, key_id=0, source=0)
    assert "V-TREE-ACYCLIC" in {d.code for d in report.errors}


def test_a_disconnected_induced_subgraph_is_rejected() -> None:
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 20, 21}, land_nodes={1, 21})
    analysis = check_induced_tree(TOPOLOGY, bitmap, source=0)
    assert not analysis.is_connected
    report = validate_route_bits(TOPOLOGY, bitmap, key_id=0, source=0)
    assert "V-TREE-CONNECTED" in {d.code for d in report.errors}


def test_land_set_completeness_and_land_count() -> None:
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 2}, land_nodes={1, 2})
    report = validate_route_bits(
        TOPOLOGY,
        bitmap,
        key_id=0,
        source=0,
        expected_land_nodes=frozenset({1}),
    )
    assert "V-LAND" in {d.code for d in report.errors}
    assert not validate_route_bits(TOPOLOGY, bitmap, key_id=0, source=0, expected_land_count=1).ok


# -- self delivery ---------------------------------------------------------


def test_self_land_bit_must_match_the_platform_trait() -> None:
    # L excludes the source, so self_delivery must be False.
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 2}, land_nodes={1, 2})
    ok = validate_route_bits(TOPOLOGY, bitmap, key_id=0, source=0, self_delivery=False)
    assert ok.ok
    bad = validate_route_bits(TOPOLOGY, bitmap, key_id=0, source=0, self_delivery=True)
    assert "V-SELFLAND" in {d.code for d in bad.errors}

    with_self = RouteBits.from_sets(NODES, pass_nodes={0, 1, 2}, land_nodes={0, 1, 2})
    assert validate_route_bits(TOPOLOGY, with_self, key_id=0, source=0, self_delivery=True).ok
    assert "V-SELFLAND" in {
        d.code
        for d in validate_route_bits(
            TOPOLOGY, with_self, key_id=0, source=0, self_delivery=False
        ).errors
    }


# -- budgets ---------------------------------------------------------------


def test_budget_accepts_the_ffn_shape() -> None:
    report = validate_table_budget(key_count=2, node_count=40)
    assert report.ok


def test_budget_rejects_more_than_sixteen_keys() -> None:
    report = validate_table_budget(key_count=17, node_count=40)
    assert "V-KEYCOUNT" in {d.code for d in report.errors}


def test_node_major_layout_uses_four_times_fewer_lines() -> None:
    key_major = validate_table_budget(key_count=16, node_count=40, node_major=False)
    node_major = validate_table_budget(key_count=16, node_count=40, node_major=True)
    # 16 lines is exactly at budget; the transposed layout needs only 4.
    assert key_major.ok
    assert node_major.ok
    assert TableLayout.KEY_MAJOR.d_cache_lines_per_core(16) == 16
    assert TableLayout.NODE_MAJOR.d_cache_lines_per_core(16) == 4
    assert TableLayout.NODE_MAJOR.d_cache_lines_per_core(2) == 1


def test_entry_layout_guard_flags_the_48_node_case() -> None:
    report = validate_entry_layout(48)
    assert not report.ok
    assert "Q1" in report.errors[0].message


# -- table level -----------------------------------------------------------


def _rows_from_specs(
    specs: list[tuple[frozenset[int], frozenset[int]]],
) -> tuple[CalendarRouteEntry, ...]:
    rows = []
    for pass_nodes, land_nodes in specs:
        bitmap = RouteBits.from_sets(NODES, pass_nodes=pass_nodes, land_nodes=land_nodes)
        rows.append(CalendarRouteEntry(route_bits=bitmap))
    return tuple(rows)


def test_table_conservation_is_checked_against_the_declared_rx_bytes() -> None:
    """expectedRxBytes must equal the land-set sum, because it is the expVal."""
    # Every core sends to a fixed 3-node group; each destination receives from 2.
    group = {0, 1, 2}
    specs = []
    rx = []
    for node in range(NODES):
        if node in group:
            pass_nodes = group
            land_nodes = group - {node}
        else:
            pass_nodes = set()
            land_nodes = set()
        specs.append((frozenset(pass_nodes), frozenset(land_nodes)))
        rx.append(100 if node in group else 0)
    geometry = PayloadGeometry(
        row_count=1,
        row_bytes=50,
        send_row_stride=50,
        recv_row_count=1,
        recv_row_stride=150,
    )
    rows = tuple(
        CalendarRouteEntry(route_bits=row.route_bits, expected_rx_bytes=rx[index])
        for index, row in enumerate(_rows_from_specs(specs))
    )
    good = build_table(
        topology=TOPOLOGY,
        definitions=(
            KeyDefinition(
                key_id=0,
                route_key=_route_key(),
                rows=rows,
                geometry=geometry,
                member_nodes=frozenset(group),
                groups={node: tuple(sorted(group)) for node in group},
                arm_lead_cycles=16,
                conflict_proof="test",
            ),
        ),
    )
    report = good.validate()
    assert report.ok, report.format()

    wrong = tuple(
        CalendarRouteEntry(
            route_bits=row.route_bits,
            expected_rx_bytes=(123 if index in group else 0),
        )
        for index, row in enumerate(rows)
    )
    bad = build_table(
        topology=TOPOLOGY,
        definitions=(
            KeyDefinition(
                key_id=0,
                route_key=_route_key(),
                rows=wrong,
                geometry=geometry,
                member_nodes=frozenset(group),
                groups={node: tuple(sorted(group)) for node in group},
                arm_lead_cycles=16,
                conflict_proof="test",
            ),
        ),
    )
    bad_report = bad.validate()
    assert "V-CONSERVE" in {d.code for d in bad_report.errors}


def test_table_rejects_a_row_count_that_does_not_cover_every_node() -> None:
    rows = _rows_from_specs([(frozenset({0}), frozenset()) for _ in range(NODES - 1)])
    with pytest.raises(CalendarError):
        build_table(
            topology=TOPOLOGY,
            definitions=(KeyDefinition(key_id=0, route_key=_route_key(), rows=rows),),
        )
