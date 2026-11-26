"""Topology, flit headers, passive forwarding, CalReg, and the NoC."""

from __future__ import annotations

import pytest

from wse_model.calendar.collective import RedOp
from wse_model.calendar.receive import DeliveryOutcome
from wse_model.calendar.route_bits import RouteBits
from wse_model.errors import CalendarError, TopologyError, WseModelError
from wse_model.noc import (
    CalRegBank,
    CalRegImage,
    CalRegSlot,
    FlitHeader,
    Noc,
    SendPlan,
    flit_count,
    simulate_multicast,
    split_payload,
)
from wse_model.noc.forward import delivery_order
from wse_model.topology import (
    CALENDAR_BASELINE,
    PROFILES,
    WHITEPAPER_HARDWARE,
    MeshTopology,
    node_name,
    topology_profile,
)

pytestmark = pytest.mark.unit

TOPOLOGY = CALENDAR_BASELINE.topology
NODES = TOPOLOGY.node_count


# -- topology --------------------------------------------------------------


def test_calendar_baseline_is_a_5x8_mesh_of_40_nodes() -> None:
    assert TOPOLOGY.rows == 5
    assert TOPOLOGY.cols == 8
    assert TOPOLOGY.node_count == 40
    assert CALENDAR_BASELINE.aicore_count == 40
    assert CALENDAR_BASELINE.io_nodes == frozenset()


def test_node_numbering_is_row_major() -> None:
    assert TOPOLOGY.node_at(0, 0) == 0
    assert TOPOLOGY.node_at(1, 0) == 8
    assert TOPOLOGY.coords(18) == (2, 2)
    assert node_name(4) == "N04"
    assert node_name(18) == "N18"


def test_neighbours_are_the_four_mesh_neighbours() -> None:
    assert TOPOLOGY.neighbours(0) == (1, 8)
    assert TOPOLOGY.neighbours(9) == (1, 8, 10, 17)
    assert TOPOLOGY.neighbours(39) == (31, 38)


def test_the_whitepaper_hardware_profile_has_48_nodes() -> None:
    assert WHITEPAPER_HARDWARE.topology.node_count == 48
    assert WHITEPAPER_HARDWARE.topology.route_bits_width_bytes == 12
    assert set(PROFILES) == {"calendar-40", "whitepaper-48", "whitepaper-48-40c"}


def test_q1_alternate_reading_keeps_48_nodes_but_40_cores() -> None:
    profile = topology_profile("whitepaper-48-40c")
    assert profile.topology.node_count == 48
    assert profile.aicore_count == 40
    assert profile.io_nodes == frozenset(range(40, 48))


def test_unknown_profile_is_rejected() -> None:
    with pytest.raises(TopologyError):
        topology_profile("nope")


def test_induced_edges_include_unintended_adjacency() -> None:
    """Calendar §2.2: any two adjacent P nodes share a live link."""
    edges = TOPOLOGY.induced_edges({0, 1, 8, 9})
    assert (0, 1) in edges
    assert (0, 8) in edges
    assert (1, 9) in edges
    assert (8, 9) in edges
    assert len(edges) == 4


def test_out_of_range_coordinates_are_rejected() -> None:
    with pytest.raises(TopologyError):
        TOPOLOGY.node_at(5, 0)
    with pytest.raises(TopologyError):
        TOPOLOGY.coords(40)


# -- flit ------------------------------------------------------------------


def test_header_is_twelve_bytes_at_40_nodes() -> None:
    """Calendar §1.8: 10 B routeBits + 6 bit opcode/redOp + 8 bit epoch = 12 B."""
    header = FlitHeader(route_bits=RouteBits.from_int(0, 40), opcode=1)
    assert header.route_bits_bytes == 10
    assert header.opcode_redop_bytes == 1
    assert header.epoch_bytes == 1
    assert header.header_bytes == 12
    assert header.payload_bytes == 52
    assert round(header.overhead_fraction * 100, 2) == 18.75


def test_header_is_fourteen_bytes_at_48_nodes() -> None:
    """Whitepaper §5.4: at 48 nodes routeBits is 96 bit and the header about 14 B."""
    header = FlitHeader(route_bits=RouteBits.from_int(0, 48), opcode=1)
    assert header.header_bytes == 14
    assert round(header.overhead_fraction * 100, 2) == 21.88


def test_header_rejects_reserved_opcodes() -> None:
    for opcode in (0, 7):
        with pytest.raises(CalendarError):
            FlitHeader(route_bits=RouteBits.from_int(0, 40), opcode=opcode)


def test_header_rejects_epoch_zero() -> None:
    with pytest.raises(WseModelError):
        FlitHeader(route_bits=RouteBits.from_int(0, 40), opcode=1, epoch_tag=0)


def test_split_payload_replicates_the_header_and_pads_the_tail() -> None:
    header = FlitHeader(route_bits=RouteBits.from_int(0, 40), opcode=1)
    flits = split_payload(b"\x01" * 100, header)
    assert len(flits) == flit_count(100, header) == 2
    assert [flit.payload_bytes for flit in flits] == [52, 48]
    assert [flit.tail_padding for flit in flits] == [0, 4]
    assert all(flit.header is header for flit in flits)
    assert sum(flit.payload_bytes for flit in flits) == 100


# -- forwarding ------------------------------------------------------------


def test_forwarding_follows_the_bitmap_path() -> None:
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 2, 3}, land_nodes={2, 3})
    trace = simulate_multicast(TOPOLOGY, bitmap, source=0)
    assert trace.land_nodes == (3, 2) or set(trace.land_nodes) == {2, 3}
    assert trace.is_clean_tree_walk
    assert trace.hop_count == 3
    assert trace.max_depth == 3
    assert delivery_order(TOPOLOGY, bitmap, source=0)[-1] in {2, 3}


def test_forwarding_replicates_at_a_fork() -> None:
    """A node with two downstream P neighbours copies the flit to both."""
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 8}, land_nodes={1, 8})
    trace = simulate_multicast(TOPOLOGY, bitmap, source=0)
    assert len(trace.landings) == 2
    assert set(trace.land_nodes) == {1, 8}
    assert trace.hop_count == 2
    assert trace.max_depth == 1


def test_forwarding_never_includes_the_ingress_port() -> None:
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 2}, land_nodes={2})
    trace = simulate_multicast(TOPOLOGY, bitmap, source=0)
    hop_at_one = next(hop for hop in trace.hops if hop.node == 1)
    assert hop_at_one.ingress == 0
    assert 0 not in hop_at_one.egress


def test_a_cycle_is_reported_as_duplicate_landings_not_an_infinite_loop() -> None:
    block = {0, 1, 8, 9}
    bitmap = RouteBits.from_sets(NODES, pass_nodes=block, land_nodes=block)
    trace = simulate_multicast(TOPOLOGY, bitmap, source=0)
    assert not trace.is_clean_tree_walk
    assert trace.duplicate_nodes
    assert trace.unexpanded_revisits


def test_link_load_counts_each_traversal() -> None:
    bitmap = RouteBits.from_sets(NODES, pass_nodes={0, 1, 2}, land_nodes={2})
    load = simulate_multicast(TOPOLOGY, bitmap, source=0).link_load()
    assert load[(0, 1)] == 1
    assert load[(1, 2)] == 1
    assert len(load) == 2


def test_forwarding_requires_p_at_the_source() -> None:
    bitmap = RouteBits.from_sets(NODES, pass_nodes={1, 2}, land_nodes={2})
    with pytest.raises(CalendarError):
        simulate_multicast(TOPOLOGY, bitmap, source=0)


# -- CalReg ----------------------------------------------------------------


def test_calreg_install_and_lookup() -> None:
    bank = CalRegBank(0)
    bank.install(CalRegSlot(opcode=1, content=b"\x01", arm_lead_cycles=64))
    assert bank.installed == (1,)
    assert bank.lookup(1).arm_lead_cycles == 64


def test_calreg_rejects_reserved_and_uninstalled_opcodes() -> None:
    bank = CalRegBank(0)
    bank.install(CalRegSlot(opcode=1, content=b"\x01"))
    with pytest.raises(CalendarError):
        bank.lookup(2)
    with pytest.raises(CalendarError):
        bank.lookup(7)
    with pytest.raises(CalendarError):
        CalRegSlot(opcode=0, content=b"")


def test_calreg_image_installs_into_every_node_atomically() -> None:
    image = CalRegImage(
        slots=(CalRegSlot(opcode=1, content=b"\x01", identities=("k0", "k1")),),
        calendar_version=7,
    )
    banks = {node: CalRegBank(node) for node in range(4)}
    image.install_into(banks, node_drained=dict.fromkeys(range(4), True))
    assert all(bank.installed == (1,) for bank in banks.values())
    assert image.slot(1).identities == ("k0", "k1")


def test_calreg_install_requires_a_drained_die() -> None:
    image = CalRegImage(slots=(CalRegSlot(opcode=1, content=b"\x01"),))
    banks = {0: CalRegBank(0)}
    with pytest.raises(CalendarError):
        image.install_into(banks, node_drained={0: False})


# -- network ---------------------------------------------------------------


def _noc() -> Noc:
    return Noc(
        TOPOLOGY,
        calreg=CalRegImage(slots=(CalRegSlot(opcode=1, content=b"\x01"),)),
    )


def test_send_delivers_to_every_land_node_and_counts_bytes() -> None:
    noc = _noc()
    for node in (0, 1, 2):
        from wse_model.calendar.receive import ReceiveAccount

        noc.ingress[node].post(
            ReceiveAccount(aicore=node, opcode=1, epoch=1, dst=0, capacity=1024, exp_val=128)
        )
    plan = SendPlan(
        source=0,
        route_bits=RouteBits.from_sets(NODES, pass_nodes={0, 1, 2}, land_nodes={1, 2}),
        opcode=1,
        epoch=1,
        row_count=1,
        row_bytes=64,
        arena_base=0,
        row_stride=256,
        source_self_off=0,
    )
    outcome = noc.send(plan)
    # The source's P bit is set but its L bit is not, so it does not land.
    assert set(outcome.land_nodes) == {1, 2}
    assert set(outcome.remote_land_nodes) == {1, 2}
    assert outcome.payload_bytes_delivered == 128
    # 64 B of rows at 52 B of payload per flit needs 2 flits, replicated over
    # the 2 tree edges.
    assert outcome.flits_injected == 2
    assert outcome.flit_hops == 4
    assert outcome.header_bytes_moved == 4 * 12
    assert outcome.wire_bytes == 4 * 64


def test_send_faults_when_the_opcode_is_not_installed() -> None:
    noc = Noc(TOPOLOGY)
    plan = SendPlan(
        source=0,
        route_bits=RouteBits.from_sets(NODES, pass_nodes={0}, land_nodes=set()),
        opcode=1,
        epoch=1,
        row_count=1,
        row_bytes=64,
        arena_base=0,
        row_stride=64,
        source_self_off=0,
    )
    with pytest.raises(CalendarError):
        noc.send(plan)


def test_self_sourced_bytes_never_count() -> None:
    """Contract 1 plus Step4s: with self_delivery off the local copy is not counted."""
    from wse_model.calendar.receive import ReceiveAccount

    noc = _noc()
    noc.ingress[0].post(
        ReceiveAccount(aicore=0, opcode=1, epoch=1, dst=0, capacity=256, exp_val=128)
    )
    noc.ingress[1].post(
        ReceiveAccount(aicore=1, opcode=1, epoch=1, dst=0, capacity=256, exp_val=64)
    )
    plan = SendPlan(
        source=0,
        route_bits=RouteBits.from_sets(NODES, pass_nodes={0, 1}, land_nodes={0, 1}),
        opcode=1,
        epoch=1,
        row_count=1,
        row_bytes=64,
        arena_base=0,
        row_stride=128,
        source_self_off=0,
        self_delivery=False,
    )
    outcome = noc.send(plan)
    assert outcome.deliveries.get(DeliveryOutcome.SELF_SOURCED.value) == 1
    assert outcome.payload_bytes_delivered == 64


def test_launch_boundary_requires_a_drained_die() -> None:
    noc = _noc()
    plan = SendPlan(
        source=0,
        route_bits=RouteBits.from_sets(NODES, pass_nodes={0, 1}, land_nodes={1}),
        opcode=1,
        epoch=1,
        row_count=1,
        row_bytes=64,
        arena_base=0,
        row_stride=64,
        source_self_off=0,
    )
    noc.send(plan)
    assert not noc.drained
    with pytest.raises(CalendarError):
        noc.install_calreg(CalRegImage(slots=(CalRegSlot(opcode=1, content=b"\x02"),)))


def test_red_op_is_carried_into_the_header() -> None:
    header = FlitHeader(route_bits=RouteBits.from_int(0, 40), opcode=2, red_op=RedOp.SUM)
    assert header.red_op is RedOp.SUM


def test_zero_length_payload_needs_no_flits() -> None:
    header = FlitHeader(route_bits=RouteBits.from_int(0, 40), opcode=1)
    assert flit_count(0, header) == 0
    assert split_payload(b"", header) == ()


def test_mesh_rejects_degenerate_dimensions() -> None:
    with pytest.raises(TopologyError):
        MeshTopology(rows=0, cols=8)


def test_table_layout_lines_helper_matches_the_layout() -> None:
    """The documented helper for the C-9 comparison."""
    from wse_model.calendar.table import TableLayout
    from wse_model.fixtures import ffn_example
    from wse_model.noc.network import table_layout_lines

    key_major = ffn_example(layout=TableLayout.KEY_MAJOR).table
    node_major = ffn_example(layout=TableLayout.NODE_MAJOR).table
    assert table_layout_lines(key_major) == 2
    assert table_layout_lines(key_major, layout=TableLayout.NODE_MAJOR) == 1
    assert table_layout_lines(node_major) == 1
    assert table_layout_lines(node_major, layout=TableLayout.KEY_MAJOR) == 2


def test_allgather_geometry_check_accepts_allgather_and_rejects_the_rest() -> None:
    from wse_model.calendar.collective import Collective
    from wse_model.calendar.geometry import SymmetricArena
    from wse_model.noc.network import allgather_geometry_check

    arena = SymmetricArena.packed(
        recv_sym_base=0, row_count=8, row_bytes=192, member_ranks=(0, 1, 2, 3)
    )
    allgather_geometry_check(collective=Collective.ALL_GATHER, geometry_rows=8, arena=arena)
    with pytest.raises(CalendarError) as excinfo:
        allgather_geometry_check(collective=Collective.REDUCE, geometry_rows=8, arena=arena)
    assert "C-10" in str(excinfo.value)
    with pytest.raises(CalendarError):
        allgather_geometry_check(collective=Collective.ALL_GATHER, geometry_rows=4, arena=arena)
