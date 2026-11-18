"""Program-interface checks; these do not execute the compiled hardware.

The representative encoding exercises route bits above node 31. Scenario
checks ensure the independent byte oracle and the range-control pair survive
serialization without relying on any hardware-model result.
"""

from dataclasses import replace

import pytest

from tools.machine_cases import bad_range, cases, tree_copy_duplicate
from wse_model.programs.abi import (
    Command,
    DeviceProfile,
    Kind,
    decode_image,
    decode_readback,
    encode_image,
    encode_readback,
)

pytestmark = pytest.mark.unit


def test_command_encoding_matches_hand_written_little_endian_record():
    command = Command(
        Kind.COPY,
        opcode=1,
        epoch=2,
        seq=0x1234,
        source=33,
        src=0x100,
        dst=0x800,
        nbytes=24,
        route=0x000C000800080038000C,
        self_copy=True,
    )
    # Sixteen literal 32-bit words, written as little-endian bytes. The route
    # occupies three words and the final 01 is the explicit self-copy flag.
    expected = bytes.fromhex(
        "04000000 00000000 01000000 02000000"
        "34120000 21000000 00010000 00080000"
        "18000000 00000000 00000000 00000000"
        "0c003800 08000800 0c000000 01000000"
    )
    assert command.pack() == expected
    assert Command.unpack(expected) == command


@pytest.mark.parametrize("case", cases().values(), ids=cases().keys())
def test_case_files_preserve_raw_input_and_readback_order(case):
    """Same profile, byte-oriented image and explicit observed spans per case."""
    profile = case.program.profile
    assert (profile.nodes, profile.bank_bytes, profile.beat_bytes, profile.seen_slots) == (
        40,
        65536,
        8,
        56,
    )
    assert decode_image(encode_image(case.patches, profile), profile) == case.patches
    assert decode_readback(encode_readback(case.readback, profile), profile) == case.readback
    assert len(case.expected_bytes) == sum(region.nbytes for region in case.readback)
    records = case.program.pack()
    assert tuple(Command.unpack(records[i : i + 64]) for i in range(0, len(records), 64)) == (
        case.program.commands
    )


def test_bad_range_changes_only_one_destination_and_preserves_input_conditions():
    """Range failure comparison fixes data, routes, capacity, count and program order."""
    good, bad = tree_copy_duplicate(), bad_range()
    assert good.patches == bad.patches
    assert good.readback == bad.readback
    assert good.program.profile == bad.program.profile
    pc = good.stall_copy_pc
    assert pc is not None
    restored = list(bad.program.commands)
    assert restored[pc].dst == 2056
    restored[pc] = replace(restored[pc], dst=2048)
    assert tuple(restored) == good.program.commands
    # All seven observed destination windows retain their explicit sentinels;
    # source node 9 retains its original 24 bytes because execution stops early.
    assert bad.expected_bytes == b"\xcc" * 280 + bytes.fromhex(
        "102132435465768798a9bacbdcedfe0f2031425364758697"
    )
    assert bad.expected_counts == {1: 0, 10: 0, 33: 0, 9: 0, 17: 0, 25: 0, 39: 0}
    assert bad.expect_fault


def test_allgather_generator_places_nonconsecutive_members_in_declared_rank_order():
    """A runtime layout with transit node 1 and two fragments per 16-byte input.

    This checks the algorithm-side command plan. Actual payload execution is
    covered separately by the shared compiled simulator, not by this test.
    """
    from wse_model.programs.allgather import allgather_program

    program = allgather_program(
        [[9, 0, 17]],
        source_address=256,
        destination_address=1536,
        bytes_per_source=16,
        fragment_bytes=8,
        routes={9: 0xC0008000B, 0: 0xC000C000A, 17: 0x8000C000B},
    )
    posts = [command for command in program.commands if command.kind == Kind.POST]
    assert {(p.node, p.dst, p.capacity, p.expected, p.compiled_expected) for p in posts} == {
        (9, 1536, 48, 32, 32),
        (0, 1536, 48, 32, 32),
        (17, 1536, 48, 32, 32),
    }
    copies = [command for command in program.commands if command.kind == Kind.COPY]
    assert len(copies) == 6
    assert {(c.source, c.seq, c.src, c.dst, c.nbytes) for c in copies} == {
        (9, 0, 256, 1536, 8),
        (9, 1, 264, 1544, 8),
        (0, 0, 256, 1552, 8),
        (0, 1, 264, 1560, 8),
        (17, 0, 256, 1568, 8),
        (17, 1, 264, 1576, 8),
    }
    expected_remote = {9: {0, 17}, 0: {9, 17}, 17: {9, 0}}
    for command in copies:
        landing_nodes = {node for node in range(40) if command.route >> (2 * node) & 1}
        assert landing_nodes == expected_remote[command.source]
        assert command.route >> 2 & 3 == 2  # Transit node 1 never receives payload.
        assert command.self_copy


@pytest.mark.parametrize("profile", [DeviceProfile(seen_slots=57), DeviceProfile(early_slots=3)])
def test_program_profile_cannot_claim_resources_absent_from_compiled_machine(profile):
    """The binary has exactly 56 identity and 2 early slots on every node."""
    with pytest.raises(ValueError):
        profile.validate()
