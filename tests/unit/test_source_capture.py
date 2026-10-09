"""Source capture reads syntax and provenance without executing a model."""

from __future__ import annotations

import ast
import codecs
import json
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file

pytestmark = pytest.mark.unit

ROOT = Path(__file__).resolve().parents[2]
SOURCE_FIXTURES = ROOT / "tests/integration/fixtures/source_language"


def _write_source(root: Path, source: str, name: str = "model.py") -> Path:
    path = root / name
    path.write_text(source, encoding="utf-8")
    return path


@pytest.mark.parametrize(
    "source_template",
    [
        "import a_module_that_must_not_be_imported_during_capture\n",
        "raise RuntimeError('capture executed the model')\n",
        "open({marker}, 'w').write('top-level write executed')\n",
        "@open({marker}, 'w').write('decorator executed')\ndef model():\n    pass\n",
        "def model(value=open({marker}, 'w').write('default executed')):\n"
        "    return value\n",
    ],
)
def test_capture_does_not_execute_python_source(
    tmp_path: Path, source_template: str
) -> None:
    marker = tmp_path / "execution-marker.txt"
    source = source_template.format(marker=repr(str(marker)))
    path = _write_source(tmp_path, source)

    captured = _capture_source_file(path, source_root=tmp_path)

    assert isinstance(captured.syntax, ast.Module)
    assert captured.source == source
    assert not marker.exists()


def test_capture_honors_declared_source_encoding(tmp_path: Path) -> None:
    path = tmp_path / "latin1.py"
    path.write_bytes(
        "# coding: latin-1\nlabel = 'caf\N{LATIN SMALL LETTER E WITH ACUTE}'\n".encode(
            "latin-1"
        )
    )

    captured = _capture_source_file(path, source_root=tmp_path)

    assert codecs.lookup(captured.encoding).name == "iso8859-1"
    assert "caf\N{LATIN SMALL LETTER E WITH ACUTE}" in captured.source
    value = captured.syntax.body[0]
    assert isinstance(value, ast.Assign)
    assert isinstance(value.value, ast.Constant)
    assert value.value.value == "caf\N{LATIN SMALL LETTER E WITH ACUTE}"


def test_capture_accepts_utf8_bom_and_reports_utf8_byte_and_codepoint_spans(
    tmp_path: Path,
) -> None:
    path = tmp_path / "bom.py"
    path.write_bytes(
        codecs.BOM_UTF8
        + "\N{GREEK SMALL LETTER PI} = '\N{LATIN SMALL LETTER E WITH ACUTE}'\n".encode()
    )

    captured = _capture_source_file(path, source_root=tmp_path)

    assert codecs.lookup(captured.encoding).name == "utf-8-sig"
    assert (
        captured.source
        == "\N{GREEK SMALL LETTER PI} = '\N{LATIN SMALL LETTER E WITH ACUTE}'\n"
    )
    assignment = captured.syntax.body[0]
    assert isinstance(assignment, ast.Assign)
    name = assignment.targets[0]
    assert isinstance(name, ast.Name)
    name_span = captured.span_for(name)
    assert (
        name_span.path,
        name_span.line,
        name_span.byte_column,
        name_span.codepoint_column,
        name_span.end_line,
        name_span.end_byte_column,
        name_span.end_codepoint_column,
    ) == (
        path.resolve(),
        1,
        1,
        1,
        1,
        3,
        2,
    )

    value_span = captured.span_for(assignment.value)
    assert (
        value_span.byte_column,
        value_span.codepoint_column,
        value_span.end_byte_column,
        value_span.end_codepoint_column,
    ) == (6, 5, 10, 8)


@pytest.mark.parametrize(
    "line_endings",
    [
        b"first = 1\r\nsecond = 2\r\nthird = 3\r\n",
        b"first = 1\rsecond = 2\rthird = 3\r",
    ],
)
def test_capture_treats_crlf_and_cr_as_python_physical_newlines(
    tmp_path: Path, line_endings: bytes
) -> None:
    path = tmp_path / "newlines.py"
    path.write_bytes(line_endings)

    captured = _capture_source_file(path, source_root=tmp_path)

    assert captured.source == "first = 1\nsecond = 2\nthird = 3\n"
    assert [statement.lineno for statement in captured.syntax.body] == [1, 2, 3]


def test_unicode_line_and_next_line_characters_are_not_python_physical_newlines(
    tmp_path: Path,
) -> None:
    source = "value = 'left\u2028middle\u0085right'\nnext_value = 1\n"
    path = _write_source(tmp_path, source)

    captured = _capture_source_file(path, source_root=tmp_path)

    first, second = captured.syntax.body
    assert isinstance(first, ast.Assign)
    assert isinstance(first.value, ast.Constant)
    assert first.value.value == "left\u2028middle\u0085right"
    assert (first.lineno, first.end_lineno, second.lineno) == (1, 1, 2)
    first_span = captured.span_for(first.value)
    second_span = captured.span_for(second)
    assert (first_span.line, first_span.end_line) == (1, 1)
    assert (second_span.line, second_span.end_line) == (2, 2)


def test_span_preserves_a_string_literal_crossing_a_physical_newline(
    tmp_path: Path,
) -> None:
    source = 'value = """first\nsecond"""\n'
    path = _write_source(tmp_path, source)
    captured = _capture_source_file(path, source_root=tmp_path)
    assignment = captured.syntax.body[0]
    assert isinstance(assignment, ast.Assign)

    span = captured.span_for(assignment.value)

    assert (
        span.line,
        span.byte_column,
        span.codepoint_column,
        span.end_line,
        span.end_byte_column,
        span.end_codepoint_column,
    ) == (1, 9, 9, 2, 10, 10)


def test_span_rejects_ast_nodes_without_source_positions(tmp_path: Path) -> None:
    path = _write_source(tmp_path, "value = 1\n")
    captured = _capture_source_file(path, source_root=tmp_path)

    with pytest.raises(ValueError, match="invalid lineno"):
        captured.span_for(captured.syntax)


def test_capture_preserves_assignment_and_function_type_comments(
    tmp_path: Path,
) -> None:
    source = (
        "value = 1  # type: int\n"
        "def identity(item):  # type: (int) -> int\n"
        "    return item\n"
    )
    path = _write_source(tmp_path, source)

    captured = _capture_source_file(path, source_root=tmp_path)

    assignment, function = captured.syntax.body
    assert isinstance(assignment, ast.Assign)
    assert isinstance(function, ast.FunctionDef)
    assert assignment.type_comment == "int"
    assert function.type_comment == "(int) -> int"


@pytest.mark.parametrize(
    "attribute, value",
    [
        ("lineno", None),
        ("lineno", True),
        ("col_offset", True),
        ("end_lineno", True),
        ("end_col_offset", True),
        ("lineno", 0),
        ("lineno", 3),
        ("end_lineno", 3),
        ("col_offset", -1),
        ("end_col_offset", -1),
        ("col_offset", 100),
        ("end_col_offset", 100),
    ],
)
def test_span_rejects_missing_wrong_type_and_out_of_range_positions(
    tmp_path: Path, attribute: str, value: object
) -> None:
    path = _write_source(tmp_path, "value = 1\n")
    captured = _capture_source_file(path, source_root=tmp_path)
    node = captured.syntax.body[0]
    setattr(node, attribute, value)

    with pytest.raises(ValueError, match="AST node"):
        captured.span_for(node)


@pytest.mark.parametrize("attribute", ["col_offset", "end_col_offset"])
def test_span_rejects_a_position_inside_a_utf8_codepoint(
    tmp_path: Path, attribute: str
) -> None:
    path = _write_source(tmp_path, "\N{GREEK SMALL LETTER PI} = 1\n")
    captured = _capture_source_file(path, source_root=tmp_path)
    assignment = captured.syntax.body[0]
    assert isinstance(assignment, ast.Assign)
    node = assignment.targets[0]
    setattr(node, attribute, 1)

    with pytest.raises(ValueError, match="Unicode code point boundary"):
        captured.span_for(node)


@pytest.mark.parametrize(
    "start_line, start_column, end_line, end_column",
    [(1, 5, 1, 4), (2, 0, 1, 1)],
)
def test_span_rejects_a_range_ending_before_it_starts(
    tmp_path: Path,
    start_line: int,
    start_column: int,
    end_line: int,
    end_column: int,
) -> None:
    path = _write_source(tmp_path, "first = 1\nsecond = 2\n")
    captured = _capture_source_file(path, source_root=tmp_path)
    node = captured.syntax.body[0]
    node.lineno = start_line
    node.col_offset = start_column
    node.end_lineno = end_line
    node.end_col_offset = end_column

    with pytest.raises(ValueError, match="ends before it starts"):
        captured.span_for(node)


def test_capture_resolves_relative_paths_inside_the_source_root(tmp_path: Path) -> None:
    source_root = tmp_path / "source"
    package = source_root / "package"
    package.mkdir(parents=True)
    path = _write_source(package, "value = 1\n")

    captured = _capture_source_file(Path("package/model.py"), source_root=source_root)

    assert captured.source_root == source_root.resolve()
    assert captured.path == path.resolve()


def test_capture_accepts_a_symlinked_source_root(tmp_path: Path) -> None:
    real_root = tmp_path / "real-source"
    real_root.mkdir()
    path = _write_source(real_root, "value = 1\n")
    linked_root = tmp_path / "linked-source"
    linked_root.symlink_to(real_root, target_is_directory=True)

    captured = _capture_source_file("model.py", source_root=linked_root)

    assert captured.source_root == real_root.resolve()
    assert captured.path == path.resolve()


def test_capture_rejects_a_source_symlink_that_escapes_the_root(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "source"
    source_root.mkdir()
    outside = tmp_path / "outside.py"
    outside.write_text("value = 1\n", encoding="utf-8")
    (source_root / "escape.py").symlink_to(outside)

    with pytest.raises(ValueError, match="escapes source root"):
        _capture_source_file("escape.py", source_root=source_root)


def test_capture_rejects_an_absolute_path_outside_the_root(tmp_path: Path) -> None:
    source_root = tmp_path / "source"
    source_root.mkdir()
    outside = tmp_path / "outside.py"
    outside.write_text("value = 1\n", encoding="utf-8")

    with pytest.raises(ValueError, match="escapes source root"):
        _capture_source_file(outside, source_root=source_root)


def test_capture_preserves_normal_missing_path_errors(tmp_path: Path) -> None:
    with pytest.raises(FileNotFoundError):
        _capture_source_file("missing.py", source_root=tmp_path)


def test_capture_requires_the_source_root_to_be_a_directory(tmp_path: Path) -> None:
    source_root = tmp_path / "not-a-directory"
    source_root.write_text("content", encoding="utf-8")

    with pytest.raises(ValueError, match="source root is not a directory"):
        _capture_source_file(source_root, source_root=source_root)


def test_source_language_fixtures_are_parse_inputs() -> None:
    expected_files = {
        "accumulator.py",
        "bank.py",
        "bank_core.py",
        "core.py",
        "packet.py",
    }

    captures = {
        path.name: _capture_source_file(path.name, source_root=SOURCE_FIXTURES)
        for path in sorted(SOURCE_FIXTURES.glob("*.py"))
    }

    assert captures.keys() == expected_files
    assert all(
        isinstance(captured.syntax, ast.Module) for captured in captures.values()
    )
    assert all(
        captured.path.parent == SOURCE_FIXTURES for captured in captures.values()
    )
    oracle = json.loads((SOURCE_FIXTURES / "expected.json").read_text())
    assert oracle["purpose"].startswith("Private independent oracle inputs")
