"""The whole flow: compile -> validate -> load -> kickstart -> launch -> collective.

This is the objective's end-to-end path in one test. It exercises the boundary
between the four parts of the model that were built separately:

* the compiler assembles a deployment package and validates it;
* the loader places the artifacts, checks the three versions, and kicks off;
* the NoC runs the FFN's two collective phases from the package's own table and
  timeslot mirror;
* the scheduler enforces the wave and drain constraints across a launch boundary.
"""

from __future__ import annotations

import pytest

from wse_model.collective import run_allgather
from wse_model.compiler import build_package
from wse_model.errors import CalendarError, VersionMismatchError
from wse_model.fixtures import clean_kernel_object, ffn_example
from wse_model.host import LaunchConstraints, Loader, Scheduler, VersionSet
from wse_model.noc import Noc

pytestmark = pytest.mark.integration


@pytest.fixture
def compiled():
    """The FFN example as a validated deployment package."""
    example = ffn_example()
    package = build_package(
        table=example.table,
        calreg=example.calreg(),
        kernel_object=clean_kernel_object(),
        weight_shard_bytes=(2048, 2048),
        conflict_proofs={0: "fixture:no-conflict-key0", 1: "fixture:no-conflict-key1"},
    )
    report = package.validate()
    assert report.ok, report.format()
    return example, package


def test_the_package_compiles_and_validates(compiled) -> None:
    example, package = compiled
    assert package.key_count == 2
    assert package.node_count == 40
    assert package.rodata_bytes == 1280
    assert package.versions.describe() == {
        "topologyVersion": 1,
        "calendarVersion": 1,
        "routeVersion": 1,
    }
    immediates = {item.key_id: item for item in package.call_site_immediates()}
    assert immediates[0].exp_val == 10752
    assert immediates[1].exp_val == 36864


def test_load_kickstart_then_run_both_phases(compiled) -> None:
    example, package = compiled

    loader = Loader()
    loader.install_kernel(rodata_bytes=package.rodata_bytes)
    loader.install_weights(shard_bytes=2048)
    loader.install_calreg(package.calreg)
    loader.check_versions(package.versions, VersionSet(1, 1, 1))
    loader.kickstart()
    assert loader.state.ready

    noc = Noc(package.table.topology, calreg=package.calreg)

    # A collective in flight blocks the launch boundary, because a straggler from
    # this launch would otherwise be counted into the next launch's epoch.
    loader.state.in_flight_transfers = 1
    with pytest.raises(CalendarError):
        loader.launch(activation_bytes=4096)
    loader.state.in_flight_transfers = 0

    phase_b = run_allgather(
        noc,
        package.table,
        0,
        geometry=example.phase_b_geometry,
        groups=example.phase_b_groups,
    )
    phase_b.check()
    phase_c = run_allgather(
        noc,
        package.table,
        1,
        geometry=example.phase_c_geometry,
        groups=example.phase_c_groups,
    )
    phase_c.check()

    assert (phase_b.epoch, phase_c.epoch) == (1, 2)
    assert noc.drained
    assert loader.launch(activation_bytes=4096) is not None
    assert loader.steady_state_dispatch_bytes(activation_bytes=4096) == 4160


def test_the_version_check_faults_before_kickstart(compiled) -> None:
    _, package = compiled
    loader = Loader()
    loader.install_kernel(rodata_bytes=package.rodata_bytes)
    loader.install_weights(shard_bytes=2048)
    loader.install_calreg(package.calreg)
    with pytest.raises(VersionMismatchError):
        loader.check_versions(package.versions, VersionSet(2, 1, 1))
    # The device is now unusable: kickstart requires a passing check.
    from wse_model.errors import WseModelError

    with pytest.raises(WseModelError):
        loader.kickstart()


def test_every_row_group_is_a_schedulable_unit(compiled) -> None:
    """The scheduler must not split a group that is syncing across waves."""
    example, package = compiled
    groups = tuple(frozenset(group) for group in sorted(set(example.phase_b_groups.values())))
    assert len(groups) == 4, "the FFN's phase B is four row groups"

    scheduler = Scheduler(node_count=package.node_count, groups=groups)
    # All four groups in one wave is fine.
    scheduler.schedule(waves=(frozenset(range(32)),), drained=True)
    assert scheduler.launches == 1

    # Splitting one row group across waves is not.
    split = frozenset(list(groups[0])[:4])
    with pytest.raises(CalendarError):
        scheduler.schedule(waves=(split, frozenset(range(32)) - split), drained=True)
    assert scheduler.launches == 1


def test_the_compiled_table_is_emitted_and_reloadable(compiled) -> None:
    """The package's .rodata is the bytes a loader would dispatch."""
    _, package = compiled
    raw = package.table.to_bytes()
    assert len(raw) == package.rodata_bytes == 1280
    assert raw[:16] == package.table.entry(0, 0).to_bytes()

    import json

    from wse_model.calendar.table import CalendarRouteTable

    reloaded = CalendarRouteTable.from_dict(json.loads(json.dumps(package.table.to_dict())))
    assert reloaded.to_bytes() == raw
    assert reloaded.validate().ok


def test_block_id_indexes_the_emitted_table(compiled) -> None:
    """The scheme's only runtime-varying index selects this core's row."""
    _, package = compiled
    for node in range(package.node_count):
        entry = package.table.entry(0, node)
        assert entry.route_bits.node_count == package.node_count
        LaunchConstraints.require_block_id(block_id=node, node_count=package.node_count)
    with pytest.raises(CalendarError):
        LaunchConstraints.require_block_id(block_id=40, node_count=package.node_count)
