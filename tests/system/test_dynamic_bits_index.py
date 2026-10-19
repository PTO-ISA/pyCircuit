"""Dynamic fixed-Bits reads reuse the existing bounded Table-index proof."""

from __future__ import annotations

from pathlib import Path

import pytest
from test_source_system_execution import _bundle, _cli, _compile, _ok, _records, _run

pytestmark = pytest.mark.system


def _unit(tmp_path: Path, name: str, source: str) -> tuple[Path, Path, Path]:
    root = tmp_path / (name + "-source")
    root.mkdir()
    path = root / (name + ".py")
    path.write_text(source, encoding="utf-8")
    unit = tmp_path / (name + "-unit")
    return root, path, unit


def _published(tmp_path: Path, name: str, source: str) -> Path:
    root, path, unit = _unit(tmp_path, name, source)
    _ok(_compile(root, path.name, unit))
    return unit


def _final(tmp_path: Path, name: str, source: str, top: str = "Top") -> Path:
    unit = _published(tmp_path, name, source)
    final = tmp_path / (name + ".ac")
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            f"checks.{name}.{top}",
            "-o",
            str(final),
        )
    )
    return final


def test_literal_bits_indices_cover_widths_1_5_65_130_and_lsb_zero(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "literal_bits",
        """import pycircuit as ac
@ac.module
def Top(one: ac.u1, five: ac.u5, wide: ac.bits[65], huge: ac.bits[130]) -> \
        tuple[ac.u1, ac.u1, ac.u1, ac.u1, ac.u1]:
    return one[0], five[0], five[4], wide[64], huge[129]
""",
    )
    text = final.read_text()
    assert text.count("#ac.math_int<1>") >= 5


def test_dynamic_full_range_and_nonpower2_bounded_index_proofs(tmp_path: Path) -> None:
    final = _final(
        tmp_path,
        "proofs",
        """from typing import Annotated
import pycircuit as ac
@ac.module
def Full(value: ac.bits[4], index: ac.u2) -> ac.u1:
    return value[index]
@ac.module
def Bounded(value: ac.bits[5], index: Annotated[int, range(8)]) -> ac.u1:
    bounded = index if index < 5 else 0
    return value[bounded]
@ac.module
def BooleanIndex(value: ac.bits[2], index: bool) -> ac.u1:
    return value[index]
@ac.module
def Top(value4: ac.bits[4], value5: ac.bits[5], index2: ac.u2,
        bounded: Annotated[int, range(8)], flag: bool) -> tuple[ac.u1, ac.u1, ac.u1]:
    return Full(value4, index2), Bounded(value5, bounded), BooleanIndex(value4[:2], flag)
""",
    )
    assert final.is_file()


def test_equivalent_closed_power_of_two_integer_domains_are_width_three(
    tmp_path: Path,
) -> None:
    unit = _published(
        tmp_path,
        "integer_domains",
        """from typing import Annotated
import pycircuit as ac
@ac.module
def Literal(value: Annotated[int, range(8)]) -> Annotated[int, range(8)]:
    return value
@ac.module
def Folded(value: Annotated[int, range(4 + 4)]) -> Annotated[int, range(4 + 4)]:
    return value
@ac.module
def Shift(value: Annotated[int, range(1 << 3)]) -> Annotated[int, range(1 << 3)]:
    return value
""",
    )
    for top in ("Literal", "Folded", "Shift"):
        final = tmp_path / (top.lower() + ".ac")
        _ok(
            _cli(
                "link",
                str(unit),
                "--top",
                f"checks.integer_domains.{top}",
                "-o",
                str(final),
            )
        )
        assert "#ac.math_int<3>" in final.read_text()


@pytest.mark.parametrize(
    "bound",
    ["5", "0", "0 - 8", "True"],
    ids=["non-power-two", "zero", "negative", "boolean"],
)
def test_invalid_integer_domain_replacement_preserves_published_unit(
    tmp_path: Path, bound: str
) -> None:
    def source(expression: str) -> str:
        annotation = f"Annotated[int, range({expression})]"
        return f"""from typing import Annotated
import pycircuit as ac
@ac.module
def Top(value: {annotation}) -> {annotation}:
    return value
"""

    root, path, unit = _unit(tmp_path, "integer_domain", source("1 << 3"))
    _ok(_compile(root, path.name, unit))
    before = {item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()}
    path.write_text(source(bound), encoding="utf-8")
    result = _cli(
        "compile",
        "-c",
        str(path),
        "--source-root",
        str(root),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    assert result.returncode != 0, result.stdout + result.stderr
    assert any(
        marker in result.stderr
        for marker in (
            "range",
            "power of two",
            "positive",
            "mathematical integer",
            "Boolean",
        )
    ), result.stderr
    assert before == {
        item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()
    }


def test_bits_index_in_helper_module_rule_and_literal_loop_contexts(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "contexts",
        """import pycircuit as ac
def helper(value: ac.bits[4], index: ac.u2) -> ac.u1:
    return value[index]
@ac.rule
def read_rule(value: ac.bits[4], index: ac.u2) -> ac.u1:
    return value[index]
@ac.module
def Top(value: ac.bits[4], index: ac.u2) -> tuple[ac.u1, ac.u1, ac.u1, ac.u1]:
    direct = value[index]
    via_helper = helper(value, index)
    via_rule = read_rule(value, index)
    loop_value = value[0]
    for ordinal in range(1):
        loop_value = value[index]
    return direct, via_helper, via_rule, loop_value
""",
    )
    assert final.is_file()


def test_bits_index_inside_table_callback_uses_same_range_proof(tmp_path: Path) -> None:
    final = _final(
        tmp_path,
        "callback",
        """import pycircuit as ac
@ac.module
def Top(value: ac.bits[4], indices: ac.table[2, ac.u2]) -> ac.table[2, ac.u1]:
    return indices.map(lambda index: value[index])
""",
    )
    assert final.is_file()


def _bit_pattern() -> str:
    symbols = ["0"] * 130
    for bit, symbol in ((0, "1"), (64, "z"), (129, "1")):
        symbols[129 - bit] = symbol
    return "".join(symbols)


def test_dynamic_index_native_and_rtl_masks_selected_z_and_unknown_high_bits(
    tmp_path: Path,
) -> None:
    pattern = _bit_pattern()
    final = _final(
        tmp_path,
        "execution",
        f"""import pycircuit as ac
@ac.module
def Read(value: ac.bits[130], index: ac.u7) -> ac.u1:
    return value[index]
@ac.system(samples=((1, 0),))
def Exercise():
    value = ac.four_state({pattern!r})
    lsb = Read(value, 0)
    selected_z = Read(value, 64)
    index_x_low = Read(value, ac.four_state('000000x'))
    index_x_high = Read(value, ac.four_state('x000000'))
    index_z_high = Read(value, ac.four_state('z000000'))
    @ac.rule
    def observe():
        ac.log('info', 'bits_index', lsb, selected_z,
               index_x_low, index_x_high, index_z_high)
    observe()
""",
        "Exercise",
    )
    expected = [
        {"kind": "bool", "value": True},
        {"kind": "bits4", "width": 1, "value": "0", "known": "0", "z": "1"},
        {"kind": "bits4", "width": 1, "value": "0", "known": "0", "z": "0"},
        {"kind": "bits4", "width": 1, "value": "0", "known": "0", "z": "0"},
        {"kind": "bits4", "width": 1, "value": "0", "known": "0", "z": "0"},
    ]
    transcripts = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        workers = (1, 2) if target == "cpp" else (1,)
        for worker in workers:
            arguments = [str(binary)] + (
                ["--workers", str(worker)] if target == "cpp" else []
            )
            result = _run(arguments)
            assert result.returncode == 0, result.stdout + result.stderr
            logs = [row for row in _records(result.stdout) if row["kind"] == "log"]
            assert len(logs) == 1
            assert logs[0]["values"] == expected
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]


def test_wide_selector_high_unknown_bit_is_not_truncated_after_range_refinement(
    tmp_path: Path,
) -> None:
    high_x = "x" + "0" * 129
    high_z = "z" + "0" * 129
    final = _final(
        tmp_path,
        "wide_selector",
        f"""import pycircuit as ac
@ac.module
def Read(value: ac.bits[4], index: ac.bits[130]) -> ac.u1:
    bounded = index if index < 4 else 0
    return value[bounded]
@ac.system(samples=((1, 0),))
def Exercise():
    selected_x = Read(ac.four_state('1010'), ac.four_state({high_x!r}))
    selected_z = Read(ac.four_state('1010'), ac.four_state({high_z!r}))
    @ac.rule
    def observe():
        ac.log('info', 'wide_selector', selected_x, selected_z)
    observe()
""",
        "Exercise",
    )
    expected = {
        "kind": "bits4",
        "width": 1,
        "value": "0",
        "known": "0",
        "z": "0",
    }
    transcripts = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        workers = (1, 2) if target == "cpp" else (1,)
        for worker in workers:
            arguments = [str(binary)] + (
                ["--workers", str(worker)] if target == "cpp" else []
            )
            result = _run(arguments)
            assert result.returncode == 0, result.stdout + result.stderr
            logs = [row for row in _records(result.stdout) if row["kind"] == "log"]
            assert len(logs) == 1
            assert logs[0]["values"] == [expected, expected]
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]


NEGATIVES = {
    "known-oob": """import pycircuit as ac
@ac.module
def Top(value: ac.bits[5]) -> ac.u1: return value[5]
""",
    "negative": """import pycircuit as ac
@ac.module
def Top(value: ac.bits[5]) -> ac.u1: return value[0 - 1]
""",
    "unproved-range": """import pycircuit as ac
@ac.module
def Top(value: ac.bits[5], index: ac.u3) -> ac.u1: return value[index]
""",
    "boolean-base": """import pycircuit as ac
@ac.module
def Top(value: bool) -> ac.u1: return value[0]
""",
    "integer-base": """from typing import Annotated
import pycircuit as ac
@ac.module
def Top(value: Annotated[int, range(32)]) -> ac.u1: return value[0]
""",
    "dynamic-slice": """import pycircuit as ac
@ac.module
def Top(value: ac.bits[5], index: ac.u3) -> ac.u1: return value[index:index + 1]
""",
    "bit-write": """import pycircuit as ac
@ac.module
def Top(value: ac.bits[5], index: ac.u3) -> ac.bits[5]:
    result = value
    result[index] = 1
    return result
""",
}


@pytest.mark.parametrize("case", sorted(NEGATIVES))
def test_bits_index_rejection_boundaries(tmp_path: Path, case: str) -> None:
    root, path, unit = _unit(tmp_path, "negative", NEGATIVES[case])
    result = _compile(root, path.name, unit)
    assert result.returncode != 0
    assert any(
        word in result.stderr
        for word in ("index", "range", "fixed bits", "slice", "assignment")
    )


def test_table_index_control_and_rejected_bits_replace_preserve_unit(
    tmp_path: Path,
) -> None:
    root, path, unit = _unit(
        tmp_path,
        "protected",
        """import pycircuit as ac
@ac.module
def Top(entries: ac.table[2, ac.u8], index: ac.u1) -> ac.u8:
    return entries[index]
""",
    )
    _ok(_compile(root, path.name, unit))
    before = {item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()}
    path.write_text(NEGATIVES["known-oob"], encoding="utf-8")
    result = _cli(
        "compile",
        "-c",
        str(path),
        "--source-root",
        str(root),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    assert result.returncode != 0
    assert before == {
        item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()
    }
