"""Host configuration for the timed communication machine.

This interface provides raw timing parameters and Calendar permissions. It
does not compute expected execution times or consume simulator results.
"""

from __future__ import annotations

import json
import struct
from collections.abc import Mapping
from dataclasses import replace
from fractions import Fraction
from pathlib import Path

from .abi import DEFAULT_PROFILE, Command, Kind, Program

_SLOTS = 8
_DIRECTIONS = 4
_CALENDAR_RECORDS = DEFAULT_PROFILE.nodes * _DIRECTIONS * _SLOTS
_PARAMETER_BASE = 4096
_RECORD = struct.Struct("<IQ")
# The driver reports 0.5 ns per tick and each directed link sends one 64 B flit per tick.
_FIXED_PARAMETERS = {"noc_clock_hz": 2_000_000_000, "link_bytes_per_noc_cycle": 64}
_PARAMETER_NAMES = (
    "ub_read_first_beat_ns",
    "tx_cdc_noc_cycles",
    "rx_cdc_noc_cycles",
    "hop_noc_cycles",
    "slot_noc_cycles",
    "alignment_noc_cycles",
    "arm_noc_cycles",
    "guard_noc_cycles",
    "ub_read_bytes_per_core_cycle",
    "ub_write_bytes_per_core_cycle",
    "payload_bytes_per_flit",
    "ub_write_first_beat_ns",
)
_FIXED_POLICIES = {
    "storage": "UB",
    "endpoint_buffering": "read and write whole packet",
    "independent_ub_ports": True,
    "packet_boundary": "one packet per SEND (original COPY)",
    "flit_pipeline": "one flit per permitted direction per NoC cycle; branch copies can use separate links",
    "injection_flits_per_noc_cycle": 1,
    "ejection_flits_per_noc_cycle": 1,
    "equal_time_order": "insertion order",
    "calendar": "eight slots; horizontal permission by source column, vertical permission by source row",
}
_MODEL_LIMITS = {
    "nodes": DEFAULT_PROFILE.nodes,
    "active_packets_per_source": 1,
    "source_queue_entries": 8,
    "source_injection_flits": 1,
    "event_heap_entries": 1024,
    "packet_buffer_bytes_per_source": DEFAULT_PROFILE.bank_bytes,
}


def load_timing_profile(path: Path) -> dict:
    return json.loads(path.read_text())


def _declared_value(entry: dict, name: str) -> object:
    if not isinstance(entry, dict):
        raise ValueError(f"timing declaration must contain value and provenance: {name}")
    source = entry.get("source")
    if (
        entry.get("status") not in ("documented", "pending")
        or not isinstance(source, str)
        or not source.strip()
        or "value" not in entry
    ):
        raise ValueError(f"missing timing value or provenance: {name}")
    return entry["value"]


def _positive_parameter(parameters: dict, name: str) -> int:
    value = _declared_value(parameters[name], name)
    if type(value) is not int or not 0 < value <= 0xFFFF_FFFF:
        raise ValueError(f"invalid timing parameter: {name}")
    return value


def fixed_calendar() -> dict[int, int]:
    """Allocate horizontal slots by source column and vertical slots by source row."""
    rows, cols = DEFAULT_PROFILE.rows, DEFAULT_PROFILE.cols
    horizontal = [sum(1 << (row * cols + col) for row in range(rows)) for col in range(cols)]
    vertical = [((1 << cols) - 1) << (row * cols) for row in range(rows)]
    vertical.extend([0] * (_SLOTS - rows))

    records = {}
    for node in range(DEFAULT_PROFILE.nodes):
        row, col = divmod(node, cols)
        for direction, valid in enumerate((row > 0, col > 0, col < cols - 1, row < rows - 1)):
            masks = horizontal if direction in (1, 2) else vertical
            base = (node * _DIRECTIONS + direction) * _SLOTS
            for slot, mask in enumerate(masks):
                records[base + slot] = mask if valid else 0
    return records


def encode_config(profile: dict, *, calendar: Mapping[int, int] | None = None) -> bytes:
    """Encode ordered little-endian address/value writes for host request kind 6.

    Explicit directed-link/slot masks override entries in the default Calendar.
    A zero mask closes that direction for the entire slot.
    """
    if profile.get("schema") != "wse-timed-machine/1":
        raise ValueError("unsupported timed machine profile")
    if profile.get("hardware_calibrated") is not False:
        raise ValueError("the timed machine profile must be marked uncalibrated")

    parameters = profile.get("parameters")
    if not isinstance(parameters, dict) or parameters.keys() != {
        *_PARAMETER_NAMES,
        *_FIXED_PARAMETERS,
        "core_clock_hz",
    }:
        raise ValueError("unexpected or missing timed machine parameter")
    for name, expected in _FIXED_PARAMETERS.items():
        if _positive_parameter(parameters, name) != expected:
            raise ValueError(f"unsupported fixed timing parameter: {name}")

    policies = profile.get("policies")
    if not isinstance(policies, dict) or policies.keys() != _FIXED_POLICIES.keys():
        raise ValueError("unexpected or missing timed machine policy")
    for name, expected in _FIXED_POLICIES.items():
        value = _declared_value(policies[name], name)
        if type(value) is not type(expected) or value != expected:
            raise ValueError(f"unsupported timed machine policy: {name}")

    limits = profile.get("model_limits")
    if (
        not isinstance(limits, dict)
        or limits.keys() != _MODEL_LIMITS.keys()
        or any(
            type(limits[name]) is not int or limits[name] != expected
            for name, expected in _MODEL_LIMITS.items()
        )
    ):
        raise ValueError("timed machine limits must match the compiled capacities")

    noc_hz = _FIXED_PARAMETERS["noc_clock_hz"]
    core_hz = _positive_parameter(parameters, "core_clock_hz")
    (
        read_ns,
        tx,
        rx,
        hop,
        slot,
        alignment,
        arm,
        guard,
        read_width,
        write_width,
        payload,
        write_ns,
    ) = (_positive_parameter(parameters, name) for name in _PARAMETER_NAMES)
    ratio = Fraction(noc_hz, core_hz)
    read_ticks = (read_ns * noc_hz + 999_999_999) // 1_000_000_000
    write_ticks = (write_ns * noc_hz + 999_999_999) // 1_000_000_000
    link_width = _FIXED_PARAMETERS["link_bytes_per_noc_cycle"]
    # A full node bank must fit in the 16-bit flit-count carrier.
    if not 2 <= payload <= link_width:
        raise ValueError("flit payload must fit both the link width and packet flit-count carrier")
    parameter_values = (
        read_ticks,
        ratio.numerator,
        ratio.denominator,
        tx,
        rx,
        hop,
        slot,
        alignment,
        arm,
        guard,
        read_width,
        write_width,
        payload,
        write_ticks,
    )
    if max(parameter_values) > 0xFFFF_FFFF:
        raise ValueError("timing configuration exceeds the 32-bit parameter carrier")
    masks = fixed_calendar()
    if calendar is not None:
        for address, mask in calendar.items():
            if (
                type(address) is not int
                or not 0 <= address < _CALENDAR_RECORDS
                or type(mask) is not int
                or not 0 <= mask < 1 << DEFAULT_PROFILE.nodes
            ):
                raise ValueError("Calendar override must fit a directional slot and 40-bit mask")
            masks[address] = mask

    return b"".join(
        _RECORD.pack(address, value)
        for address, value in (*masks.items(), *enumerate(parameter_values, _PARAMETER_BASE))
    )


def timed_program(program: Program) -> Program:
    """Queue each COPY as one packet and release each posted AllGather phase.

    The AllGather generator emits one contiguous POST block followed by one
    contiguous COPY block per phase. All other commands retain their order.
    """
    commands = []
    previous_kind = None
    for command in program.commands:
        if command.kind == Kind.COPY:
            if previous_kind == Kind.POST:
                commands.append(Command(Kind.BARRIER))
            elif previous_kind != Kind.COPY:
                raise ValueError("AllGather copies must follow a contiguous POST block")
            commands.append(replace(command, kind=Kind.SEND))
        else:
            commands.append(command)
        previous_kind = command.kind
    result = replace(program, commands=tuple(commands))
    result.pack()
    return result
