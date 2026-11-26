"""The Batcher, the UB bus, and the load/launch/runtime tiers."""

from __future__ import annotations

import pytest

from wse_model.errors import CalendarError, OpenItemError, VersionMismatchError, WseModelError
from wse_model.host import (
    FABRIC_LANES,
    HOST_LANES,
    TOTAL_LANES,
    BatcherMem,
    BatcherResponsibility,
    BatcherSpec,
    DispatchChain,
    InstallState,
    LaunchConstraints,
    Loader,
    RuntimeTier,
    Scheduler,
    UbBus,
    UbLane,
    VersionSet,
    dispatch_chains,
    dispatch_paths,
)
from wse_model.host.batcher import (
    BATCHER_MEM_PATH,
    DATA_CACHE_LINE_BYTES,
    INSTRUCTION_BLOCK_BYTES,
)
from wse_model.host.ub_bus import local_dram_over_fabric_ratio
from wse_model.noc.calreg import CalRegImage, CalRegSlot

pytestmark = pytest.mark.unit


# -- UB bus ----------------------------------------------------------------


def test_the_bus_has_eight_fabric_lanes_and_one_host_lane() -> None:
    assert (FABRIC_LANES, HOST_LANES, TOTAL_LANES) == (8, 1, 9)
    bus = UbBus()
    assert bus.describe()["total_lanes"] == 9
    assert UbLane.FABRIC.gb_per_s == 224.0
    assert UbLane.HOST.gb_per_s == 14.0


def test_over_subscribing_the_lane_budget_is_rejected() -> None:
    UbBus(fabric_lanes=9, host_lanes=0)
    with pytest.raises(WseModelError):
        UbBus(fabric_lanes=9, host_lanes=1)


def test_bus_transfer_time_and_the_weight_residency_premise() -> None:
    bus = UbBus()
    assert bus.transfer_seconds(224_000_000_000) == pytest.approx(1.0)
    assert bus.bytes_in(1.0) == pytest.approx(224e9)
    # Whitepaper §6.3: 224 GB/s on the fabric against 1 TB/s per core.
    assert local_dram_over_fabric_ratio() == pytest.approx(4.464, rel=1e-3)
    # Streaming the weights would cost more than reading them locally, which is
    # why they must reside.
    dram_seconds = 1_000_000_000 / 1.0e12
    assert bus.weights_can_reside_only(weight_bytes=1_000_000_000, local_dram_seconds=dram_seconds)


# -- Batcher ---------------------------------------------------------------


def test_the_four_responsibilities_and_their_frequencies() -> None:
    paths = {path.responsibility: path for path in dispatch_paths()}
    assert set(paths) == set(BatcherResponsibility)
    assert paths[BatcherResponsibility.KERNEL_BINARY].frequency.value == "once per kernel"
    assert paths[BatcherResponsibility.WEIGHTS].frequency.value == "once per model load"
    assert paths[BatcherResponsibility.ACTIVATIONS].frequency.value == "every inference"
    assert paths[BatcherResponsibility.RESULTS].responsibility.direction == "out"
    assert paths[BatcherResponsibility.ACTIVATIONS].responsibility.direction == "in"
    assert "local DRAM" in paths[BatcherResponsibility.WEIGHTS].destination


def test_the_cache_refill_path_avoids_local_dram_and_l1() -> None:
    assert "never through local DRAM" in BATCHER_MEM_PATH
    assert "never through L1" in BATCHER_MEM_PATH
    assert INSTRUCTION_BLOCK_BYTES == 2048
    assert DATA_CACHE_LINE_BYTES == 64


def test_batcher_spec_is_undeclared_by_default_and_gates_on_q3() -> None:
    spec = BatcherSpec()
    assert not spec.is_declared
    assert spec.describe()["open_item"] == "Q3"
    with pytest.raises(OpenItemError) as excinfo:
        spec.require(consumer="unit test")
    assert "Q3" in str(excinfo.value)
    with pytest.raises(OpenItemError):
        spec.dispatch_seconds(1024)


def test_a_declared_batcher_spec_produces_timings() -> None:
    spec = BatcherSpec(
        mem_capacity_bytes=1024 * 1024,
        mem_bandwidth_bytes_per_sec=2.0e9,
        cores_per_batcher=4,
        parallel_channels=2,
    )
    assert spec.is_declared
    assert spec.dispatch_seconds(2_000_000_000) == pytest.approx(1.0)
    with pytest.raises(WseModelError):
        spec.dispatch_seconds(-1)


def test_refill_split_matches_the_calendar_account() -> None:
    """Calendar §3.8.4: 20 distinct lines, 4 cores each, only 20 reach DDR."""
    account = BatcherMem().data_refill(distinct_lines=20, requesters_per_line=4)
    assert account.requests == 80
    assert account.ddr_backed_requests == 20
    assert account.mem_served_requests == 60
    assert account.ddr_bytes == 20 * 64
    assert account.mem_served_bytes == 60 * 64
    described = account.describe()
    assert described["requests"] == 80 and described["ddr_backed_requests"] == 20


def test_instruction_refill_uses_two_kilobyte_blocks() -> None:
    account = BatcherMem().instruction_refill(distinct_blocks=3, requesters_per_line=40)
    assert account.line_bytes == 2048
    assert account.requests == 120
    assert account.ddr_backed_requests == 3


def test_batcher_mem_capacity_check_requires_q3() -> None:
    mem = BatcherMem()
    with pytest.raises(OpenItemError):
        mem.capacity_check(resident_bytes=1)
    declared = BatcherMem(
        spec=BatcherSpec(
            mem_capacity_bytes=4096,
            mem_bandwidth_bytes_per_sec=1.0e9,
            cores_per_batcher=4,
            parallel_channels=1,
        )
    )
    declared.capacity_check(resident_bytes=4096)
    with pytest.raises(WseModelError):
        declared.capacity_check(resident_bytes=4097)


def test_weight_dispatch_must_be_page_blocked() -> None:
    mem = BatcherMem()
    mem.weight_page_check(weight_bytes=2048)
    with pytest.raises(WseModelError) as excinfo:
        mem.weight_page_check(weight_bytes=1000)
    assert "page" in str(excinfo.value)


# -- runtime ---------------------------------------------------------------


def test_the_three_frequency_tiers() -> None:
    assert {tier.name for tier in RuntimeTier} == {"LOAD", "PER_LAUNCH", "RUNTIME"}
    chains = dispatch_chains()
    assert chains[DispatchChain.KERNEL_BINARY]["tier"] is RuntimeTier.LOAD
    assert chains[DispatchChain.ACTIVATIONS]["tier"] is RuntimeTier.PER_LAUNCH


def test_calreg_is_the_only_chain_that_bypasses_the_batcher() -> None:
    chains = dispatch_chains()
    assert chains[DispatchChain.CALREG]["via_batcher"] is False
    assert chains[DispatchChain.CALREG]["destination"] == "NoC node timeslot registers"
    assert chains[DispatchChain.KERNEL_BINARY]["via_batcher"] is True
    assert chains[DispatchChain.WEIGHTS]["through_local_dram"] is True
    assert chains[DispatchChain.KERNEL_BINARY]["through_local_dram"] is False
    assert chains[DispatchChain.KERNEL_BINARY]["through_l1"] is False


def test_version_check_faults_on_any_mismatch() -> None:
    compiled = VersionSet(topology_version=1, calendar_version=2, route_version=3)
    compiled.check(VersionSet(1, 2, 3))
    for chip in (
        VersionSet(2, 2, 3),
        VersionSet(1, 3, 3),
        VersionSet(1, 2, 4),
    ):
        with pytest.raises(VersionMismatchError) as excinfo:
            compiled.check(chip)
        assert "must fault" in str(excinfo.value) or "version check failed" in str(excinfo.value)


def _loaded_loader() -> Loader:
    loader = Loader()
    loader.install_kernel(rodata_bytes=1280)
    loader.install_weights(shard_bytes=2048)
    loader.install_calreg(CalRegImage(slots=(CalRegSlot(opcode=1, content=b"\x01"),)))
    loader.check_versions(VersionSet(1, 1, 1), VersionSet(1, 1, 1))
    state = loader.kickstart()
    assert state is RuntimeTier.LOAD
    return loader


def test_the_load_sequence_must_complete_before_kickstart() -> None:
    loader = Loader()
    with pytest.raises(WseModelError):
        loader.kickstart()
    loader.install_kernel()
    with pytest.raises(WseModelError):
        loader.kickstart()
    loader.install_weights(shard_bytes=2048)
    loader.install_calreg(CalRegImage(slots=(CalRegSlot(opcode=1, content=b"\x01"),)))
    loader.check_versions(VersionSet(1, 1, 1), VersionSet(1, 1, 1))
    loader.kickstart()
    assert loader.state.ready


def test_weights_must_be_page_aligned_at_install() -> None:
    loader = Loader()
    with pytest.raises(WseModelError):
        loader.install_weights(shard_bytes=1000)


def test_calreg_install_requires_a_quiet_die() -> None:
    loader = Loader()
    loader.state.in_flight_transfers = 3
    with pytest.raises(CalendarError) as excinfo:
        loader.install_calreg(CalRegImage(slots=(CalRegSlot(opcode=1, content=b"\x01"),)))
    assert "stop-the-world" in str(excinfo.value)


def test_launch_requires_a_drained_die_and_sends_no_calendar_data() -> None:
    """Whitepaper §12.2: steady state dispatches activations plus a descriptor."""
    loader = _loaded_loader()
    loader.state.in_flight_transfers = 1
    with pytest.raises(CalendarError) as excinfo:
        loader.launch(activation_bytes=4096)
    assert "drained" in str(excinfo.value)

    loader.state.in_flight_transfers = 0
    assert loader.launch(activation_bytes=4096) is RuntimeTier.PER_LAUNCH
    assert loader.steady_state_dispatch_bytes(activation_bytes=4096) == 4160
    # Nothing per-launch carries route or timeslot data.
    chains = dispatch_chains()
    assert DispatchChain.ACTIVATIONS in chains
    assert chains[DispatchChain.CALREG]["tier"] is RuntimeTier.LOAD


def test_launch_before_load_is_rejected() -> None:
    with pytest.raises(WseModelError):
        Loader().launch(activation_bytes=1)


# -- scheduling constraints ------------------------------------------------


def test_a_wave_may_not_split_a_syncing_group() -> None:
    groups = (frozenset({0, 1, 2, 3}),)
    LaunchConstraints.check_wave(waves=(groups[0],), groups=groups)
    with pytest.raises(CalendarError) as excinfo:
        LaunchConstraints.check_wave(waves=(frozenset({0, 1}), frozenset({2, 3})), groups=groups)
    assert "splits a syncing group" in str(excinfo.value)


def test_two_independent_groups_may_share_a_wave() -> None:
    groups = (frozenset({0, 1}), frozenset({2, 3}))
    LaunchConstraints.check_wave(waves=(frozenset({0, 1, 2, 3}),), groups=groups)


def test_a_phase_barrier_requires_a_drain() -> None:
    LaunchConstraints.require_phase_barrier(drained=True)
    with pytest.raises(CalendarError) as excinfo:
        LaunchConstraints.require_phase_barrier(drained=False)
    assert "SW-1" in str(excinfo.value)


def test_requirements_for_the_calreg_window_and_block_id() -> None:
    LaunchConstraints.require_calreg_window(in_flight=0)
    with pytest.raises(CalendarError):
        LaunchConstraints.require_calreg_window(in_flight=1)
    LaunchConstraints.require_block_id(block_id=39, node_count=40)
    with pytest.raises(CalendarError):
        LaunchConstraints.require_block_id(block_id=40, node_count=40)


def test_block_id_from_the_spr_costs_no_load() -> None:
    spr = LaunchConstraints.block_id_cost(from_spr=True)
    arg = LaunchConstraints.block_id_cost(from_spr=False)
    assert spr["loads"] == 0 and spr["value_class"] == "A"
    assert arg["loads"] == 1 and arg["value_class"] == "B"


def test_scheduler_enforces_both_constraints_and_counts_launches() -> None:
    scheduler = Scheduler(groups=(frozenset({0, 1, 2, 3}),))
    scheduler.schedule(waves=(frozenset({0, 1, 2, 3}),), drained=True)
    assert scheduler.launches == 1
    with pytest.raises(CalendarError):
        scheduler.schedule(waves=(frozenset({0, 1}), frozenset({2, 3})), drained=True)
    assert scheduler.launches == 1
    with pytest.raises(CalendarError):
        scheduler.schedule(waves=(frozenset({0, 1, 2, 3}),), drained=False)


def test_install_state_reports_readiness() -> None:
    state = InstallState()
    assert not state.ready and state.drained
    state.in_flight_transfers = 2
    assert not state.drained
    assert state.describe()["ready"] is False
