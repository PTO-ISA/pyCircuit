"""Ordinary expression composition retains types, snapshots and four-state data."""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def _program(tmp_path: Path, text: str, provider: str | None = None) -> Path:
    source = tmp_path / "source"
    source.mkdir()
    units = []
    if provider is not None:
        (source / "provider.py").write_text(provider)
        unit = tmp_path / "provider"
        _ok(_compile(source, "provider.py", unit))
        # Consumer compilation must use the published helper interface/closure.
        (source / "provider.py").rename(source / "provider.py.hidden")
        units.append(unit)
    (source / "bench.py").write_text(text)
    unit = tmp_path / "bench"
    _ok(_compile(source, "bench.py", unit, units))
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link",
            *map(str, [*units, unit]),
            "--top",
            "checks.bench.Exercise",
            "-o",
            str(final),
        )
    )
    return final


def _symbols(value: dict, expected_width: int) -> str:
    # Known observations use the integer/bool schema without a width; bits4
    # observations carry their full value/known/Z representation explicitly.
    width = value.get("width", expected_width)
    bits4 = value["kind"] == "bits4"
    data = int(value["value"], 16) if bits4 else int(value["value"])
    assert 0 <= data < (1 << width), value
    known = int(value["known"], 16) if bits4 else (1 << width) - 1
    high_z = int(value["z"], 16) if bits4 else 0
    return "".join(
        (
            str((data >> bit) & 1)
            if known & (1 << bit)
            else "z" if high_z & (1 << bit) else "x"
        )
        for bit in reversed(range(width))
    )


def _execute(
    final: Path,
    tmp_path: Path,
    expected: dict[str, tuple[str, ...] | list[tuple[str, ...]]],
    *,
    cycles: int = 1,
):
    traces = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        if target == "verilog":
            backend = json.loads(Path(str(binary) + ".backend.json").read_text())
            assert backend["backend"] == "iverilog"
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", str(cycles), "--workers", str(workers)]
                if target == "cpp"
                else [f"+cycles={cycles}"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            logs = [row for row in rows if row["kind"] == "log"]
            assert {row["spec"]["event"] for row in logs} == set(expected)
            for event, values in expected.items():
                samples = (
                    values if isinstance(values, list) else [values] * (2 * cycles)
                )
                selected = [row for row in logs if row["spec"]["event"] == event]
                assert [int(row["evaluation_epoch"]) for row in selected] == list(
                    range(2 * cycles)
                )
                assert [
                    tuple(
                        _symbols(value, len(symbols))
                        for value, symbols in zip(row["values"], sample, strict=True)
                    )
                    for row, sample in zip(selected, samples, strict=True)
                ] == samples
            traces.append(logs)
    assert traces[0] == traces[1] == traces[2]


def test_rebinding_snapshots_follow_changing_module_inputs(tmp_path):
    text = """import pycircuit as ac
@ac.module
def Wires(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    value: ac.u16 = large
    return ac.concat(saved, value)

@ac.system
def Exercise():
    phase: ac.u8 = 1
    current = Wires(phase, ac.concat(phase, phase + 1))
    @ac.rule
    def advance():
        value = phase
        saved = value
        value = current
        ac.log("info", "snapshots", saved, value, phase + 3)
        phase = phase + 1
"""
    # Reannotation must keep the existing local a wire: the second cycle sees
    # the new input immediately, while saved locals retain their earlier value.
    _execute(
        _program(tmp_path, text),
        tmp_path,
        {
            "snapshots": [("00000001", "000000010000000100000010", "00000100")] * 2
            + [("00000010", "000000100000001000000011", "00000101")] * 2,
        },
        cycles=2,
    )


def test_published_tuple_helper_preserves_struct_snapshots_and_four_state(tmp_path):
    provider = """import pycircuit as ac
@ac.struct
class Packet:
    data: ac.u8
    tag: ac.u4

def split(value: ac.u8) -> tuple[Packet, ac.u4]:
    return Packet(data=value, tag=value[:4]), ac.popcount(value)

def summarize(value: ac.u8) -> tuple[Packet, ac.u4]:
    packet, count = split(value)
    total: ac.u4 = 0
    for index in range(2):
        packet, count = split(packet.data)
        total = total + count
    return packet, total
"""
    text = """import pycircuit as ac
from checks.provider import summarize
@ac.system
def Exercise():
    @ac.rule
    def observe():
        packet, total = summarize(ac.four_state("10100101"))
        saved = packet
        packet.data = 7
        ac.log("info", "known", saved.data, saved.tag, packet.data, total)
        mixed, mixed_total = summarize(ac.four_state("10xz0011"))
        ac.log("info", "mixed", mixed.data, mixed.tag, mixed_total)
"""
    _execute(
        _program(tmp_path, text, provider),
        tmp_path,
        {
            "known": ("10100101", "0101", "00000111", "1000"),
            "mixed": ("10xz0011", "0011", "xxxx"),
        },
    )


def test_table_callbacks_compose_helpers_and_lexical_snapshots(tmp_path):
    text = """import pycircuit as ac
def key(value: ac.u8) -> ac.u4:
    return value[:4]

def eligible(value: ac.u8) -> bool:
    return key(value) >= 3

@ac.system
def Exercise():
    rows = ac.table[3, ac.u8](init=0)
    phase: ac.u2 = 0
    bias = ac.four_state("0001")

    def shifted(value: ac.u4) -> ac.u4:
        return value + bias

    bias = ac.four_state("0010")

    def relay(value: ac.u4) -> ac.u4:
        return shifted(value)

    bias = ac.four_state("0011")

    def transform(values: ac.table[3, ac.u8]) -> ac.u4:
        mapped = values.map(lambda row: relay(key(row)) + bias)
        return mapped[0]

    @ac.rule
    def update_and_query():
        value = ac.four_state("00001001")
        if phase == 1:
            value = ac.four_state("10xz0011")
        rows[0] = value
        rows[1] = 6
        rows[2] = 2
        mapped = rows.map(lambda row: relay(key(row)) + bias)
        first, present = rows.first(where=lambda row: eligible(row))
        best, found = rows.argmin(where=lambda row: eligible(row), key=lambda row: key(row))
        ac.log("info", "mapped", mapped[0], mapped[1], mapped[2], transform(rows))
        ac.log("info", "selected", rows[first], present, rows[best], found)
        phase = phase + 1
"""
    # shifted keeps bias=1 through relay's definition at bias=2; the lambda
    # sees bias=3. Queries use known low bits and preserve the selected X/Z data.
    _execute(
        _program(tmp_path, text),
        tmp_path,
        {
            "mapped": [("1101", "1010", "0110", "1101")] * 2
            + [("0111", "1010", "0110", "0111")] * 2,
            "selected": [("00001001", "1", "00000110", "1")] * 2
            + [("10xz0011", "1", "10xz0011", "1")] * 2,
        },
        cycles=2,
    )
