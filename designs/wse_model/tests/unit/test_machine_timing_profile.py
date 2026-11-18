"""Check the loader's raw ABI without relying on the simulated implementation."""

import struct
from dataclasses import replace
from pathlib import Path

import pytest

from wse_model.programs.abi import Command, Kind, Program
from wse_model.programs.timing import encode_config, load_timing_profile, timed_program

PROFILE = Path(__file__).resolve().parents[2] / "models/machine/timing_profile.json"


def test_fixed_calendar_and_clock_parameter_carrier() -> None:
    profile = load_timing_profile(PROFILE)
    records = dict(struct.iter_unpack("<IQ", encode_config(profile)))
    assert set(records) == {*range(1280), *range(4096, 4110)}
    # Node 0 has no up or left neighbors. Its right link uses source columns,
    # and its down link uses source rows.
    assert [records[address] for address in range(16)] == [0] * 16
    assert records[17] == sum(1 << node for node in (1, 9, 17, 25, 33))
    assert records[25] == 0xFF00
    assert records[29] == 0
    assert [records[address] for address in range(4096, 4110)] == [
        200,
        10,
        7,
        2,
        2,
        1,
        1024,
        16,
        8,
        1,
        128,
        128,
        52,
        200,
    ]


def test_custom_calendar_and_write_latency_are_independent() -> None:
    profile = load_timing_profile(PROFILE)
    baseline = dict(struct.iter_unpack("<IQ", encode_config(profile)))
    profile["parameters"]["ub_write_first_beat_ns"]["value"] = 101
    changed = dict(struct.iter_unpack("<IQ", encode_config(profile, calendar={16: 1 << 33})))
    assert {address: value for address, value in changed.items() if baseline[address] != value} == {
        16: 1 << 33,
        4109: 202,
    }


@pytest.mark.parametrize(
    ("path", "replacement", "message"),
    (
        (("parameters", "noc_clock_hz", "value"), 1_000_000_000, "noc_clock_hz"),
        (("parameters", "link_bytes_per_noc_cycle", "value"), 128, "link_bytes_per_noc_cycle"),
        (("policies", "independent_ub_ports", "value"), False, "independent_ub_ports"),
        (("policies", "ejection_flits_per_noc_cycle", "value"), 2, "ejection_flits_per_noc_cycle"),
        (("policies", "injection_flits_per_noc_cycle", "source"), "", "provenance"),
        (("model_limits", "event_heap_entries"), 2048, "compiled capacities"),
    ),
)
def test_profile_rejects_unsupported_hardware_declarations(path, replacement, message) -> None:
    profile = load_timing_profile(PROFILE)
    entry = profile
    # Change one field from the supported profile per case.
    for name in path[:-1]:
        entry = entry[name]
    entry[path[-1]] = replacement
    with pytest.raises(ValueError, match=message):
        encode_config(profile)


def test_timed_program_preserves_each_copy_as_a_separate_packet() -> None:
    install = Command(Kind.INSTALL, node=0, opcode=1)
    post = Command(Kind.POST, node=1, opcode=1, epoch=1, dst=32, capacity=16, expected=16)
    copy = Command(Kind.COPY, opcode=1, epoch=1, src=8, dst=32, nbytes=8, route=14, self_copy=True)
    next_fragment = replace(copy, seq=1, src=16, dst=40)
    wait = Command(Kind.WAIT, node=1, opcode=1, epoch=1)
    retire = Command(Kind.RETIRE, node=1)
    end = Command(Kind.END)
    program = Program((install, post, copy, next_fragment, wait, retire, post, copy, wait, end))

    timed = timed_program(program)

    assert timed.commands == (
        install,
        post,
        Command(Kind.BARRIER),
        replace(copy, kind=Kind.SEND),
        replace(next_fragment, kind=Kind.SEND),
        wait,
        retire,
        post,
        Command(Kind.BARRIER),
        replace(copy, kind=Kind.SEND),
        wait,
        end,
    )
    assert program.commands[2] == copy
