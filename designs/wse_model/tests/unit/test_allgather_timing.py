"""One-edge analytical controls; hold topology/rate fixed in each comparison."""

import pytest

from wse_model.analysis.allgather_timing import link_occupancy_bounds
from wse_model.compiler.allgather import Descriptor

pytestmark = pytest.mark.unit


def segment(source=0, fragment=0, nbytes=24, phase=0):
    # Source pair P=1/L=0; destination pair P=1/L=1.
    route = 14 if source == 0 else 11
    return Descriptor(
        phase,
        source,
        fragment,
        nbytes,
        fragment * nbytes,
        4096 + fragment * nbytes,
        route,
        nbytes,
        4096,
        128,
    )


def test_opposite_directions_have_independent_link_capacity():
    result = link_occupancy_bounds(
        [segment(), segment(source=1)], epoch_tag_bits=8, packet_boundary="source"
    )
    assert result["phases"][0]["bottleneck_noc_cycles"] == 1
    assert result["phases"][0]["wire_byte_hops"] == 128
    assert result["sequential_phases_link_lower_bound_seconds"] == 0.5e-9


def test_packet_boundary_only_changes_tail_padding():
    descriptors = [segment(), segment(fragment=1)]
    merged = link_occupancy_bounds(descriptors, epoch_tag_bits=8, packet_boundary="source")
    split = link_occupancy_bounds(descriptors, epoch_tag_bits=8, packet_boundary="fragment")
    assert merged["phases"][0]["bottleneck_noc_cycles"] == 1
    assert split["phases"][0]["bottleneck_noc_cycles"] == 2


def test_header_width_and_sequential_phase_boundaries():
    descriptors = [segment(nbytes=52), segment(nbytes=52, phase=1)]
    small = link_occupancy_bounds(descriptors, epoch_tag_bits=8, packet_boundary="source")
    large = link_occupancy_bounds(descriptors, epoch_tag_bits=16, packet_boundary="source")
    assert small["header"]["header_bytes"] == 12
    assert large["header"]["header_bytes"] == 13
    assert small["sequential_phases_link_lower_bound_seconds"] == 1e-9
    assert large["sequential_phases_link_lower_bound_seconds"] == 2e-9
