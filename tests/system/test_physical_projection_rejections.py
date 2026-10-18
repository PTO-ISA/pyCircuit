"""Physical projection rejects isolated mutations of verified source final IR."""

from __future__ import annotations

import os
from pathlib import Path

import pytest
from test_physical_rtl_export import SOURCE
from test_source_system_execution import _cli, _compile, _ok, _run

pytestmark = pytest.mark.system


def _split_top(text: str) -> list[str]:
    result, start, depth = [], 0, 0
    pairs = {"(": ")", "[": "]", "{": "}", "<": ">"}
    closers = set(pairs.values())
    stack = []
    for index, char in enumerate(text):
        if char in pairs:
            stack.append(pairs[char])
        elif char in closers:
            assert stack and stack.pop() == char
        elif char == "," and not stack:
            result.append(text[start:index].strip())
            start = index + 1
        depth = len(stack)
    assert depth == 0
    result.append(text[start:].strip())
    return result


def _module_span(lines: list[str], symbol: str) -> tuple[int, int]:
    start = next(i for i, line in enumerate(lines) if f'sym_name = "{symbol}"' in line)
    end = next(
        (i for i in range(start + 1, len(lines)) if '"ac.module"()' in lines[i]),
        len(lines) - 1,
    )
    return start, end


def _block_arguments(line: str) -> list[tuple[str, str]]:
    body = line[line.index("(") + 1 : line.rindex("):")]
    result = []
    for raw in _split_top(body):
        name, separator, type_and_location = raw.partition(": ")
        assert separator
        # The location attribute contains nested `loc(...)`; its first ` loc(`
        # separator follows the complete type.
        type_text, separator, _ = type_and_location.partition(" loc(")
        assert separator
        result.append((name, type_text))
    return result


def _instance_operands(line: str) -> list[str]:
    marker = '"ac.instance"('
    begin = line.index(marker) + len(marker)
    depth = 0
    for end in range(begin, len(line)):
        if line[end] in "([{<":
            depth += 1
        elif line[end] in ")]}>":
            if line[end] == ")" and depth == 0:
                return _split_top(line[begin:end])
            depth -= 1
    raise AssertionError("unterminated instance operands")


def _source_final(tmp_path: Path, top: str = "Pair") -> Path:
    source = tmp_path / "source"
    source.mkdir()
    (source / "design.py").write_text(SOURCE, encoding="utf-8")
    unit = tmp_path / "unit"
    _ok(_compile(source, "design.py", unit))
    final = tmp_path / (top + ".ac")
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.design." + top,
            "-o",
            str(final),
        )
    )
    return final


def _ordinary_verified(path: Path) -> None:
    _ok(_run([os.environ["PYCIRCUIT_LINKER"], "--design", str(path), "--verify-only"]))


def _snapshot(directory: Path) -> dict[str, bytes]:
    return {
        path.relative_to(directory).as_posix(): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file()
    }


def _physical_rejected(path: Path, tmp_path: Path, diagnostic: str) -> None:
    output = tmp_path / "verilog"
    _ok(_cli("emit", str(path), "--target", "verilog", "-o", str(output)))
    before = _snapshot(output)
    result = _cli(
        "emit",
        str(path),
        "--target",
        "verilog",
        "--rtl-mode",
        "physical",
        "-o",
        str(output),
        "--replace",
    )
    assert result.returncode != 0, result.stdout + result.stderr
    assert diagnostic in result.stdout + result.stderr
    assert _snapshot(output) == before


def _mixed_table_mutation(path: Path, *, derived_clock: bool) -> None:
    lines = path.read_text(encoding="utf-8").splitlines()
    start, end = _module_span(lines, "checks.design.Pair")
    block = next(
        i for i in range(start + 1, end) if lines[i].lstrip().startswith("^bb0")
    )
    arguments = _block_arguments(lines[block])
    clock, clock_type = arguments[-2]
    reset, _ = arguments[-1]
    _, data_type = arguments[1]
    first = next(
        i
        for i in range(block + 1, end)
        if '"ac.instance"' in lines[i] and "@checks.design.Cell" in lines[i]
    )
    shape = '#ac.static_expr<{kind = "literal", location = {path = "design.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @checks.design.Pair, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>'
    aliases = [
        f"#test_w2 = {shape}",
        "#test_w0 = " + shape.replace("#ac.math_int<2>", "#ac.math_int<0>"),
        f"!test_b1 = {clock_type}",
        f"!test_b8 = {data_type}",
        "!test_t2b1 = !ac.table<[#test_w2], !test_b1>",
    ]
    module_line = next(i for i, line in enumerate(lines) if line.startswith("module "))
    lines[module_line:module_line] = aliases
    shift = len(aliases)
    first += shift
    selected_clock = "%test_selected" if derived_clock else clock
    operations = [
        '    %test_index = "ac.bits.constant"() {value = #test_w0} : () -> !test_b1',
        f'    %test_controls = "ac.table.create"({clock}, {reset}) : (!test_b1, !test_b1) -> !test_t2b1',
        '    %test_selected, %test_range = "ac.table.get"(%test_controls, %test_index) : (!test_t2b1, !test_b1) -> (!test_b1, !test_b1)',
        '    %test_wide = "ac.bits.resize"(%test_selected) {mode = "zext"} : (!test_b1) -> !test_b8',
    ]
    operands = _instance_operands(lines[first])
    operands[-2] = selected_clock
    prefix = lines[first][
        : lines[first].index('"ac.instance"(') + len('"ac.instance"(')
    ]
    suffix = lines[first][lines[first].index(")", prefix.__len__()) :]
    lines[first] = prefix + ", ".join(operands) + suffix
    lines[first:first] = operations
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _dynamic_initializer_mutation(path: Path, symbol: str) -> None:
    lines = path.read_text(encoding="utf-8").splitlines()
    start, end = _module_span(lines, symbol)
    block = next(
        i for i in range(start + 1, end) if lines[i].lstrip().startswith("^bb0")
    )
    dynamic, dynamic_type = _block_arguments(lines[block])[0]
    occurrence = next(
        i
        for i in range(start + 1, end)
        if '"ac.instance"' in lines[i]
        and (
            "@pycircuit.__builtins__.dff" in lines[i]
            or "@pycircuit.__builtins__.dffe" in lines[i]
        )
    )
    operands = _instance_operands(lines[occurrence])
    annotation = lines[occurrence]
    type_begin = annotation.index(": (") + 3
    type_end = annotation.index(") ->", type_begin)
    initial_type = _split_top(annotation[type_begin:type_end])[-1]
    low = f'#ac.static_expr<{{kind = "literal", location = {{path = "design.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}, origin = {{site = {{definition = @{symbol}, ast_path = []}}, expansion = []}}, value = {{kind = "integer", value = #ac.math_int<0>}}}}>'
    conversion = f'    %test_dynamic_init = "ac.bits.extract"({dynamic}) {{low = {low}}} : ({dynamic_type}) -> {initial_type}'
    operands[-1] = "%test_dynamic_init"
    begin = lines[occurrence].index('"ac.instance"(') + len('"ac.instance"(')
    finish = lines[occurrence].index(")", begin)
    lines[occurrence] = (
        lines[occurrence][:begin] + ", ".join(operands) + lines[occurrence][finish:]
    )
    lines.insert(occurrence, conversion)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def test_dead_mixed_table_clock_data_use_rejects_physical_projection(
    tmp_path: Path,
) -> None:
    final = _source_final(tmp_path)
    _mixed_table_mutation(final, derived_clock=False)
    _ordinary_verified(final)
    _physical_rejected(
        final,
        tmp_path,
        "physical RTL forbids clock use as data or combinational control",
    )


def test_table_derived_clock_rejects_actual_root_domain_pair(tmp_path: Path) -> None:
    final = _source_final(tmp_path)
    _mixed_table_mutation(final, derived_clock=True)
    _ordinary_verified(final)
    _physical_rejected(
        final,
        tmp_path,
        "physical RTL requires all sampling and storage to use the actual root clock/reset pair",
    )


def test_unselected_dynamic_initializer_does_not_poison_safe_root(
    tmp_path: Path,
) -> None:
    final = _source_final(tmp_path)
    _dynamic_initializer_mutation(final, "checks.design.GuardedCell")
    _ordinary_verified(final)
    output = tmp_path / "physical-safe"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )


def test_selected_dynamic_initializer_rejects_closed_static_requirement(
    tmp_path: Path,
) -> None:
    final = _source_final(tmp_path, "GuardedPair")
    _dynamic_initializer_mutation(final, "checks.design.GuardedCell")
    _ordinary_verified(final)
    _physical_rejected(
        final,
        tmp_path,
        "state initializer requires a closed static literal/constructor DAG",
    )
