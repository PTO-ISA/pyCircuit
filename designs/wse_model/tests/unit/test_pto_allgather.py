"""Bounded 32-member rank-major lowering; no PyPTO or simulation is run here."""

import struct
from collections import Counter

import pytest

from wse_model.calendar.route_bits import RouteBits
from wse_model.compiler.allgather import lower_calendar
from wse_model.noc.forward import simulate_multicast
from wse_model.topology import CALENDAR_BASELINE

pytestmark = pytest.mark.unit


@pytest.fixture
def lowering_inputs():
    """Two contiguous FP32 kernels, fixed groups and distinct source/B/C buffers."""
    phases = [
        {
            "name": "row",
            "kernel": "gather",
            "ranks": 8,
            "words": 384,
            "groups": [list(range(start, start + 8)) for start in range(0, 32, 8)],
            "source_base": 0,
            "target_base": 4096,
            "input_shape": [1, 384],
            "output_shape": [8, 384],
        },
        {
            "name": "column",
            "kernel": "gather",
            "ranks": 4,
            "words": 3072,
            "groups": [list(range(column, 32, 8)) for column in range(8)],
            "source_base": 4096,
            "target_base": 16384,
            "input_shape": [1, 3072],
            "output_shape": [4, 3072],
        },
    ]
    for phase in phases:
        phase.update(dtype="f32", layout="rank-major-contiguous", fragments=8)
    program = {
        "schema": "wse-pypto-allgather/1",
        "topology": [5, 8],
        "members": list(range(32)),
        "bank_bytes": 65536,
        "phases": phases,
        "dependency": {
            "producer": "row.target",
            "consumer": "column.source",
            "view": "contiguous-flatten",
            "bytes": 12288,
        },
    }
    transfers = [
        [
            {
                "source_rank": source,
                "target_rank": target,
                "source_offset": 0,
                "target_offset": source * chunk_bytes,
                "nbytes": chunk_bytes,
            }
            for source in range(ranks)
            for target in range(ranks)
        ]
        for ranks, chunk_bytes in ((8, 1536), (4, 12288))
    ]
    return program, transfers


def test_two_phase_layout_routes_and_remote_byte_conservation(lowering_inputs):
    descriptors = lower_calendar(*lowering_inputs)
    assert [(d.phase, d.source, d.fragment) for d in descriptors] == [
        (phase, source, fragment)
        for phase in range(2)
        for source in range(32)
        for fragment in range(8)
    ]
    received = Counter()
    for d in descriptors:
        if d.phase == 0:
            rank = d.source % 8
            group = set(range(d.source // 8 * 8, d.source // 8 * 8 + 8))
            chunk, fragment_bytes, source_base, target_base = 1536, 192, 0, 4096
            expected, capacity = 10752, 12288
        else:
            rank = d.source // 8
            group = set(range(d.source % 8, 32, 8))
            chunk, fragment_bytes, source_base, target_base = 12288, 1536, 4096, 16384
            expected, capacity = 36864, 49152
        assert d.source_address == source_base + d.fragment * fragment_bytes
        assert d.destination_address == target_base + rank * chunk + d.fragment * fragment_bytes
        assert (d.nbytes, d.expected, d.arena_base, d.arena_capacity) == (
            fragment_bytes,
            expected,
            target_base,
            capacity,
        )
        assert (
            target_base
            <= d.destination_address
            < d.destination_address + d.nbytes
            <= target_base + capacity
        )
        route = RouteBits.from_int(d.route, node_count=40)
        trace = simulate_multicast(CALENDAR_BASELINE.topology, route, source=d.source)
        assert set(trace.land_nodes) == group - {d.source}
        assert len(trace.land_nodes) == len(group) - 1
        for destination in trace.land_nodes:
            received[d.phase, destination] += d.nbytes
    assert received == Counter(
        {
            (phase, node): expected
            for phase, expected in ((0, 10752), (1, 36864))
            for node in range(32)
        }
    )


def test_descriptor_binary_is_twelve_little_endian_u32_fields(lowering_inputs):
    descriptors = lower_calendar(*lowering_inputs)
    payload = b"".join(d.pack() for d in descriptors)
    assert len(payload) == 512 * 12 * 4
    rows = list(struct.iter_unpack("<12I", payload))
    # Frozen wire-order examples derive from the workload, not Descriptor.pack.
    assert rows[0] == (0, 0, 0, 192, 0, 4096, 0xFFFE, 0, 0, 10752, 4096, 12288)
    assert rows[256] == (1, 0, 0, 1536, 4096, 16384, 0x00030002, 0x00030003, 0, 36864, 16384, 49152)
    for d, row in zip(descriptors, rows, strict=True):
        assert row[:6] == (
            d.phase,
            d.source,
            d.fragment,
            d.nbytes,
            d.source_address,
            d.destination_address,
        )
        assert row[6] | row[7] << 32 | row[8] << 64 == d.route
        assert row[8] < 1 << 16  # Exact 80-bit route in its three-word container.
        assert row[9:] == (d.expected, d.arena_base, d.arena_capacity)


def test_rejects_one_wrong_transfer_offset(lowering_inputs):
    program, transfers = lowering_inputs
    # Hold shape, groups and all other transfers fixed; move one destination word.
    transfers[0][1]["target_offset"] += 4
    with pytest.raises(ValueError):
        lower_calendar(program, transfers)


def test_rejects_wrong_phase_dependency(lowering_inputs):
    program, transfers = lowering_inputs
    # Valid copies remain unchanged; only the producer relation is wrong.
    program["dependency"]["producer"] = "row.source"
    with pytest.raises(ValueError):
        lower_calendar(program, transfers)
