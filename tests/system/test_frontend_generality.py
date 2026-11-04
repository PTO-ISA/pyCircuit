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


def test_count_builtins_compose_in_bounded_helper_loops(tmp_path):
    text = """import pycircuit as ac
def counts(value: ac.u8) -> tuple[ac.u4, ac.u4, ac.u4]:
    return ac.count_leading_zeros(value), ac.count_trailing_zeros(value), ac.popcount(value)

def loop_counts(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8]:
    leading, trailing, population = counts(value)
    for index in range(2):
        leading, trailing, population = counts(value)
        direct_leading = ac.count_leading_zeros(value)
        direct_trailing = ac.count_trailing_zeros(value)
        direct_population = ac.popcount(value)
    return ac.concat(direct_leading, leading), ac.concat(direct_trailing, trailing), ac.concat(direct_population, population)

@ac.system
def Exercise():
    @ac.rule
    def observe():
"""
    expected = {}
    # Explicit counts cover the zero sentinel, both bit orders and unknown data.
    for name, bits, counts in (
        ("zero", "00000000", (8, 8, 0)),
        ("middle", "00101000", (2, 3, 2)),
        ("ones", "11111111", (0, 0, 8)),
        ("unknown", "xxxxxxxx", None),
        ("high_z", "zzzzzzzz", None),
    ):
        text += f'        value_{name} = ac.four_state("{bits}")\n'
        text += f"        leading, trailing, population = loop_counts(value_{name})\n"
        text += f'        ac.log("info", "{name}", leading, trailing, population)\n'
        expected[name] = (
            tuple(f"{count:04b}" * 2 for count in counts)
            if counts is not None
            else ("xxxxxxxx",) * 3
        )
    _execute(_program(tmp_path, text), tmp_path, expected)


def test_observation_direct_expressions_match_saved_values(tmp_path):
    text = """import pycircuit as ac
@ac.rule
def negative_result():
    return 0 - 1

@ac.rule
def add_two(value):
    return value + 2

@ac.rule
def replace_negative(value) -> None:
    value = 0 - 4

@ac.rule
def forwarded(value) -> tuple[ac.u8, ac.u8]:
    saved = value
    replace_negative(value)
    return saved + 5, value + 5

@ac.rule
def direct(value) -> tuple[ac.u8, ac.u8]:
    saved = value
    value = 0 - 4
    return saved + 5, value + 5

@ac.module
def Observe(value: ac.u8, flag: ac.u1) -> ac.u8:
    added = value + 3
    selected = value if flag else ~value
    @ac.rule
    def observe_module_locals():
        ac.log("info", "module", value + 3, added, value if flag else ~value, selected)
    @ac.rule
    def observe():
        added = value + 3
        selected = value if flag else ~value
        ac.log("info", "rule", value + 3, added, value if flag else ~value, selected)
        integer = 255 + 1
        forward = 1 if flag else 1023
        reverse = 1023 if flag else 1
        ac.log("info", "integers", 255 + 1, integer, 1 if flag else 1023, forward, 1023 if flag else 1, reverse)
    return selected

@ac.system
def Exercise():
    known = Observe(ac.four_state("00101000"), ac.four_state("1"))
    @ac.rule
    def observe():
        result: ac.u8 = negative_result() + 2
        formal: ac.u8 = add_two(0 - 1)
        before, after = forwarded(0 - 3)
        direct_before, direct_after = direct(0 - 3)
        ac.log("info", "negative", result, formal, before, after, direct_before, direct_after)
        value = ac.four_state("10xz0011")
        flag = ac.four_state("z")
        added = value + 3
        selected = value if flag else value
        ac.log("info", "four_state", value + 3, added, value if flag else value, selected)
        forward = 1 if flag else 1023
        reverse = 1023 if flag else 1
        ac.log("info", "integer_join", 1 if flag else 1023, forward, 1023 if flag else 1, reverse)
"""
    expected = {
        # Negative results/formals retain signed facts; value-only actuals have no owner.
        "negative": tuple(f"{value:08b}" for value in (1, 1, 2, 2, 2, 1)),
        "module": ("00101011", "00101011", "00101000", "00101000"),
        "rule": ("00101011", "00101011", "00101000", "00101000"),
        "integers": ("100000000",) * 2 + ("0000000001",) * 2 + ("1111111111",) * 2,
        "four_state": ("xxxxxxxx", "xxxxxxxx", "10xz0011", "10xz0011"),
        "integer_join": ("xxxxxxxxx1",) * 4,
    }
    _execute(_program(tmp_path, text), tmp_path, expected)


def test_unannotated_rebinding_and_saved_values_across_callable_bodies(tmp_path):
    text = """import pycircuit as ac
def helper(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    value = large
    return ac.concat(saved, value)

def loop_rebind(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    for index in range(2):
        value = large
    return ac.concat(saved, value)

@ac.module
def Rebind(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    value = large
    return ac.concat(saved, value)

@ac.rule
def rule_rebind(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    value = large
    return ac.concat(saved, value)

@ac.rule
def rule_boundary(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    value: ac.u16 = large
    value: ac.u16 = large
    return ac.concat(saved, value)

@ac.module
def Operations() -> None:
    @ac.method
    def rebind(small: ac.u8, large: ac.u16) -> ac.u24:
        value = small
        saved = value
        value = large
        return ac.concat(saved, value)

    @ac.method
    def boundary(small: ac.u8, large: ac.u16) -> ac.u24:
        value = small
        saved = value
        value: ac.u16 = large
        value: ac.u16 = large
        return ac.concat(saved, value)

def branch(small: ac.u8, large: ac.u16, flag: ac.u1) -> ac.u24:
    value = small
    saved = value
    if flag:
        value = large
    else:
        value = ~large
    return ac.concat(saved, value)

def boundary(small: ac.u8, large: ac.u16) -> ac.u16:
    value = small
    value: ac.u16 = large
    value: ac.u16 = small
    return value

def retained(value: ac.u8, flag: ac.u1) -> ac.u8:
    result = value
    if flag:
        result = ~value
    return result

@ac.system
def Exercise():
    owner = Operations()
    module_value = Rebind(ac.four_state("10100101"), ac.four_state("1011111011101111"))
    @ac.rule
    def observe():
        small = ac.four_state("10100101")
        large = ac.four_state("1011111011101111")
        first = helper(small, large)
        second = rule_rebind(small, large)
        third = owner.rebind(small, large)
        rule_annotated = rule_boundary(small, large)
        method_annotated = owner.boundary(small, large)
        ac.log("info", "rebind", first, module_value, second, third, loop_rebind(small, large))
        ac.log("info", "boundary", boundary(small, large), rule_annotated, method_annotated)
        ac.log("info", "branches", branch(small, large, ac.four_state("0")), branch(small, large, ac.four_state("1")))
        ac.log("info", "unknown", branch(small, large, ac.four_state("x")), branch(small, large, ac.four_state("z")))
        ac.log("info", "missing_arm", retained(small, ac.four_state("0")), retained(small, ac.four_state("1")), retained(small, ac.four_state("x")), retained(small, ac.four_state("z")))
        local = ac.four_state("10xz0011")
        saved = local
        local = large
        untouched = helper(saved, local)
        ac.log("info", "snapshots", saved, local, untouched, first)
"""
    packed = f"{0xA5BEEF:024b}"
    _execute(
        _program(tmp_path, text),
        tmp_path,
        {
            "rebind": (packed,) * 5,
            "boundary": (f"{0xA5:016b}", packed, packed),
            "branches": (f"{0xA54110:024b}", packed),
            "unknown": ("10100101" + "x" * 16,) * 2,
            "missing_arm": ("10100101", "01011010", "xxxxxxxx", "xxxxxxxx"),
            "snapshots": (
                "10xz0011",
                f"{0xBEEF:016b}",
                "10xz0011" + f"{0xBEEF:016b}",
                packed,
            ),
        },
    )


def test_module_reannotation_keeps_existing_wires_combinational(tmp_path):
    text = """import pycircuit as ac
@ac.module
def Wires(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    value: ac.u16 = large
    value: ac.u16 = large
    if small == 1:
        partial: ac.u16 = large
    partial = large + 1
    @ac.rule
    def observe():
        ac.log("info", "module_locals", saved, value, partial)
    return ac.concat(saved, value)

@ac.module
def OwnedWires(small: ac.u8, large: ac.u16) -> ac.u24:
    value = small
    saved = value
    value: ac.u16 = large
    value: ac.u16 = large
    @ac.method
    def read() -> ac.u24:
        return ac.concat(saved, value)
    return ac.concat(saved, value)

@ac.system
def Exercise():
    phase: ac.u8 = 1
    plain = Wires(phase, ac.concat(phase, phase + 1))
    owner = OwnedWires(phase, ac.concat(phase, phase + 1))
    @ac.rule
    def drive():
        observed = owner.read()
        ac.log("info", "current", plain, owner, observed)
        phase = phase + 1
"""
    # Each pair of Work samples sees the current phase. An accidental storage
    # declaration or delayed capture would lose the immediate input change.
    current = []
    locals_seen = []
    for phase in (1, 2, 3):
        large = (phase << 8) | (phase + 1)
        packed = f"{(phase << 16) | large:024b}"
        current.extend([(packed,) * 3] * 2)
        locals_seen.extend([(f"{phase:08b}", f"{large:016b}", f"{large + 1:016b}")] * 2)
    _execute(
        _program(tmp_path, text),
        tmp_path,
        {"current": current, "module_locals": locals_seen},
        cycles=3,
    )


@pytest.mark.parametrize(
    "body",
    [
        "    value: ac.u8 = small\n    value = large\n",
        "    if flag:\n        value = large\n",
    ],
    ids=["explicit_boundary", "missing_branch"],
)
def test_local_rebinding_keeps_explicit_and_branch_boundaries(tmp_path, body):
    source = tmp_path / "source"
    source.mkdir()
    (source / "bad.py").write_text(
        "import pycircuit as ac\n"
        "def helper(small: ac.u8, large: ac.u16, flag: ac.u1) -> ac.u16:\n"
        + body
        + "    return value\n"
        "@ac.module\n"
        "def Top(small: ac.u8, large: ac.u16, flag: ac.u1) -> ac.u16:\n"
        "    return helper(small, large, flag)\n"
    )
    destination = tmp_path / "unit"
    result = _compile(source, "bad.py", destination)
    assert result.returncode != 0
    assert "error:" in result.stderr
    assert "Assertion failed" not in result.stderr
    assert not destination.exists()


def test_instance_handle_cannot_be_reannotated_as_an_ordinary_local(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    (source / "bad.py").write_text(
        """import pycircuit as ac
@ac.module
def Child() -> ac.u8:
    return 7
@ac.module
def Top() -> ac.u8:
    child = Child()
    child: ac.u8 = 3
    return child
"""
    )
    destination = tmp_path / "unit"
    result = _compile(source, "bad.py", destination)
    assert result.returncode != 0
    assert "error:" in result.stderr
    assert "Assertion failed" not in result.stderr
    assert not destination.exists()


def test_helper_tuple_cannot_be_used_as_a_scalar_result(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    (source / "bad.py").write_text(
        """import pycircuit as ac
def pair(value: ac.u8) -> tuple[ac.u4, ac.u4]:
    return value[:4], value[4:]
@ac.module
def Top(value: ac.u8) -> ac.u8:
    return pair(value)
"""
    )
    destination = tmp_path / "unit"
    result = _compile(source, "bad.py", destination)
    assert result.returncode != 0
    assert "error:" in result.stderr
    assert "Assertion failed" not in result.stderr
    assert not destination.exists()


def test_imported_helpers_compose_tuples_callbacks_and_lexical_snapshots(tmp_path):
    provider = """import pycircuit as ac
@ac.struct
class Packet:
    data: ac.u8
    tag: ac.u4

def split(value: ac.u8) -> tuple[Packet, ac.u8]:
    return Packet(data=value, tag=value[:4]), ~value

def forward(value: ac.u8) -> tuple[Packet, ac.u8]:
    packet, inverted = split(value)
    for index in range(2):
        packet, inverted = split(packet.data)
    return packet, inverted

def make_packet(value: ac.u8) -> Packet:
    return Packet(data=value, tag=value[:4])

def key(value: ac.u8) -> ac.u4:
    return value[:4]

def eligible(value: ac.u8) -> bool:
    return key(value) >= 3
"""
    text = """import pycircuit as ac
from checks.provider import Packet, forward, make_packet, key, eligible
@ac.module
def Decode(value: ac.u8) -> tuple[Packet, ac.u8]:
    packet, inverted = forward(value)
    return packet, inverted

@ac.system
def Exercise():
    module_packet, module_inverted = Decode(ac.four_state("10100101"))
    module_xz, module_inverted_xz = Decode(ac.four_state("10xz0011"))
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
    def observe():
        changed, inverted = forward(ac.four_state("10100101"))
        old = changed
        changed.data = 7
        ac.log("info", "known", old.data, old.tag, inverted, changed.data)
        packet_xz, inverted_xz = forward(ac.four_state("10xz0011"))
        ac.log("info", "four_state", packet_xz.data, packet_xz.tag, inverted_xz)
        ac.log("info", "module_known", module_packet.data, module_packet.tag, module_inverted)
        ac.log("info", "module_four_state", module_xz.data, module_xz.tag, module_inverted_xz)
        value = ac.four_state("00001001")
        if phase == 1:
            value = ac.four_state("10xx0011")
        if phase == 2:
            value = ac.four_state("10zz0011")
        rows[0] = value
        rows[1] = 6
        rows[2] = 2
        mapped = rows.map(lambda row: relay(key(row)) + bias)
        packets = rows.map(lambda row: make_packet(row))
        first, present = rows.first(where=lambda row: eligible(row))
        best, found = rows.argmin(where=lambda row: eligible(row), key=lambda row: key(row))
        ac.log("info", "mapped", mapped[0], mapped[1], mapped[2], transform(rows))
        ac.log("info", "packet", packets[0].data, packets[0].tag)
        ac.log("info", "queries", first, present, best, found)
        phase = phase + 1
"""
    # The helper retains bias=1; the lambda reads bias=3. The intermediary
    # declaration at bias=2 must not replace the transitive callee snapshot.
    _execute(
        _program(tmp_path, text, provider),
        tmp_path,
        {
            "known": ("10100101", "0101", "01011010", "00000111"),
            "four_state": ("10xz0011", "0011", "01xx1100"),
            "module_known": ("10100101", "0101", "01011010"),
            "module_four_state": ("10xz0011", "0011", "01xx1100"),
            "mapped": [("1101", "1010", "0110", "1101")] * 2
            + [("0111", "1010", "0110", "0111")] * 4,
            "packet": [("00001001", "1001")] * 2
            + [("10xx0011", "0011")] * 2
            + [("10zz0011", "0011")] * 2,
            "queries": [("00", "1", "01", "1")] * 2 + [("00", "1", "00", "1")] * 4,
        },
        cycles=3,
    )


@pytest.mark.parametrize("case", ["generic_row", "retained_operation"])
def test_table_lambda_helper_keeps_generic_types_and_operation_boundary(tmp_path, case):
    helper = """def helper(value: ac.u4) -> ac.u4:
    return value
"""
    if case == "retained_operation":
        helper = """@ac.struct
class Group:
    values: ac.table[2, ac.u8]
    tag: ac.u8

def reduce(value: ac.u8) -> ac.u8:
    group = Group(values=(value, value), tag=value)
    return group.values.fold(kind="add")

def helper(value: ac.u8) -> ac.u8:
    return reduce(value)
"""
    source = tmp_path / "source"
    source.mkdir()
    (source / "bad.py").write_text(
        "import pycircuit as ac\n"
        + helper
        + """@ac.module
def Top() -> ac.u8:
    rows = ac.table[3, ac.u8](init=1)
    @ac.rule
    def evaluate() -> ac.u8:
        values = rows.map(lambda row: helper(row))
        return values[0]
    return evaluate
"""
    )
    destination = tmp_path / "unit"
    result = _compile(source, "bad.py", destination)
    assert result.returncode != 0
    assert "error:" in result.stderr
    assert "Assertion failed" not in result.stderr
    assert not destination.exists()
