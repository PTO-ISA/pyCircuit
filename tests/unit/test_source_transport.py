"""Private Python capture transport is parseable data, never model execution."""

from __future__ import annotations

import ast
import sys
from dataclasses import replace
from decimal import Decimal
from pathlib import Path

import pycircuit
import pytest
from pycircuit._source_capture import _capture_source_file, _CapturedSourceFile
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.unit


def _capture_text(
    root: Path, source: str, name: str = "model.py"
) -> _CapturedSourceFile:
    path = root / name
    path.write_text(source, encoding="utf-8")
    return _capture_source_file(path, source_root=root)


def _emit_transport(captured: _CapturedSourceFile) -> str:
    return _emit_source_transport(captured)


def _span_text(captured: _CapturedSourceFile, node: ast.AST) -> str:
    span = captured.span_for(node)
    return (
        "{"
        f"start_line = {span.line} : i64, "
        f"start_byte_column = {span.byte_column} : i64, "
        f"start_codepoint_column = {span.codepoint_column} : i64, "
        f"end_line = {span.end_line} : i64, "
        f"end_byte_column = {span.end_byte_column} : i64, "
        f"end_codepoint_column = {span.end_codepoint_column} : i64"
        "}"
    )


def test_transport_emits_exactly_one_captured_file_without_scanning_or_execution(
    tmp_path: Path,
) -> None:
    marker = tmp_path / "executed.txt"
    source = (
        "import sibling\n"
        f"open({str(marker)!r}, 'w').write('executed')\n"
        "raise RuntimeError('model execution')\n"
    )
    captured = _capture_text(tmp_path, source)
    (tmp_path / "sibling.py").write_text("SIBLING_SENTINEL = )\n", encoding="utf-8")

    transport = _emit_transport(captured)

    assert not marker.exists()
    assert transport.count('kind = "Module"') == 1
    assert 'path = "model.py"' in transport
    assert "SIBLING_SENTINEL" not in transport


def test_basic_pycircuit_import_does_not_call_old_semantic_compiler(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def forbidden(*_args: object, **_kwargs: object) -> None:
        raise AssertionError("old semantic compiler was called")

    monkeypatch.setattr(pycircuit, "compile_cycle_aware", forbidden, raising=False)
    captured = _capture_text(
        tmp_path,
        "from pycircuit import module\n\n@module\nclass Leaf:\n    pass\n",
    )

    transport = _emit_transport(captured)

    assert 'kind = "ImportFrom"' in transport
    assert 'kind = "ClassDef"' in transport


def test_huge_hex_integer_uses_independent_canonical_decimal_oracle_without_global_change(
    tmp_path: Path,
) -> None:
    get_digit_limit = getattr(sys, "get_int_max_str_digits", lambda: 0)
    digit_limit = get_digit_limit()
    hexadecimal = "F" * 5000
    value = int(hexadecimal, 16)
    expected = format(Decimal(value), "f")
    if digit_limit:
        assert len(expected) > digit_limit
        with pytest.raises(ValueError, match="Exceeds the limit"):
            str(value)
    captured = _capture_text(tmp_path, f"VALUE = 0x{hexadecimal}\n")

    transport = _emit_transport(captured)

    assert f'integer = "{expected}"' in transport
    assert get_digit_limit() == digit_limit


def test_literal_kinds_remain_distinct_for_native_validation(tmp_path: Path) -> None:
    captured = _capture_text(
        tmp_path,
        "TRUTH = True\n"
        "FALSEHOOD = False\n"
        "FLOATING = 1.5\n"
        "COMPLEX = 2j\n"
        "BINARY = b'\\x00\\xff'\n"
        "ELLIPSIS_VALUE = ...\n",
    )

    transport = _emit_transport(captured)

    assert "value = true" in transport
    assert "value = false" in transport
    assert '["float", "0x1.8000000000000p+0"]' in transport
    assert '["complex", "0x0.0p+0", "0x1.0000000000000p+1"]' in transport
    assert '["bytes", "00ff"]' in transport
    assert '["ellipsis"]' in transport
    assert 'value = {integer = "1"}' not in transport


def test_mlir_byte_escaping_covers_path_and_literal_bytes(tmp_path: Path) -> None:
    captured = _capture_text(
        tmp_path,
        'VALUE = "quote\\" slash\\\\ nul\\x00 ctrl\\x01 piπ sep\u2028"\n',
        "escaped.py",
    )
    captured = replace(captured, path=tmp_path / 'quo"te-π.py')

    transport = _emit_transport(captured)

    assert 'path = "quo\\22te-\\CF\\80.py"' in transport
    assert "quote\\22" in transport
    assert "slash\\5C" in transport
    assert "nul\\00" in transport
    assert "ctrl\\01" in transport
    assert "pi\\CF\\80" in transport
    assert "sep\\E2\\80\\A8" in transport
    assert "π" not in transport
    assert " " not in transport


def test_lf_and_crlf_sources_have_identical_transport(tmp_path: Path) -> None:
    path = tmp_path / "model.py"
    logical_source = "first = 'é'\nsecond = 2\n"
    path.write_bytes(logical_source.encode("utf-8"))
    lf_transport = _emit_transport(_capture_source_file(path, source_root=tmp_path))
    path.write_bytes(logical_source.replace("\n", "\r\n").encode("utf-8"))
    crlf_transport = _emit_transport(_capture_source_file(path, source_root=tmp_path))

    assert crlf_transport == lf_transport
    assert "start_line = 2 : i64" in crlf_transport


def test_unicode_separators_do_not_split_physical_line_spans(tmp_path: Path) -> None:
    captured = _capture_text(
        tmp_path, "value = 'left\u2028middle\u0085right'\nnext_value = 1\n"
    )
    first, second = captured.syntax.body
    assert isinstance(first, ast.Assign)
    assert isinstance(first.value, ast.Constant)

    transport = _emit_transport(captured)

    constant_offset = transport.index('kind = "Constant"')
    constant_tail = transport[constant_offset : constant_offset + 600]
    assert "start_line = 1 : i64" in constant_tail
    assert "end_line = 1 : i64" in constant_tail
    assert _span_text(captured, second) in transport


def test_positionless_helper_nodes_inherit_their_parent_span(tmp_path: Path) -> None:
    captured = _capture_text(tmp_path, "def add_one(value):\n    return value + 1\n")
    function = captured.syntax.body[0]
    assert isinstance(function, ast.FunctionDef)
    return_node = function.body[0]
    assert isinstance(return_node, ast.Return)
    expression = return_node.value
    assert isinstance(expression, ast.BinOp)
    name = expression.left
    assert isinstance(name, ast.Name)

    transport = _emit_transport(captured)

    assert (
        f'{{kind = "Load", fields = {{}}, span = {_span_text(captured, name)}}}'
        in transport
    )
    assert (
        f'{{kind = "Add", fields = {{}}, span = {_span_text(captured, expression)}}}'
        in transport
    )


@pytest.mark.parametrize(
    "attribute, value",
    [
        ("lineno", None),
        ("col_offset", True),
        ("end_lineno", 0),
        ("end_col_offset", -1),
    ],
)
def test_located_ast_nodes_with_missing_or_malformed_positions_are_rejected(
    tmp_path: Path, attribute: str, value: object
) -> None:
    captured = _capture_text(tmp_path, "value = 1\n")
    assignment = captured.syntax.body[0]
    setattr(assignment, attribute, value)

    with pytest.raises(ValueError, match="AST node"):
        _emit_source_transport(captured)


def test_located_ast_node_with_reversed_span_is_rejected(tmp_path: Path) -> None:
    captured = _capture_text(tmp_path, "value = 1\n")
    assignment = captured.syntax.body[0]
    assignment.col_offset = 5
    assignment.end_col_offset = 4

    with pytest.raises(ValueError, match="ends before it starts"):
        _emit_source_transport(captured)
