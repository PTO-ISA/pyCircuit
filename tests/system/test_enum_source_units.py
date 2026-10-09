"""Real declaration units, saved-final provenance and protected public outputs.

This lane transports nominal declarations only. Enum value/default/tuple and
runtime behavior have independent owners; synthetic maps are not authority.
"""

from __future__ import annotations

import ast
import hashlib
import json
import re
import shutil
from pathlib import Path

import pytest
import test_public_emit as public
import test_source_map as maps

pytestmark = pytest.mark.system
PACKAGE = "enum_source_units"
SOURCES = {
    "enums.py": "from enum import Enum\nimport pycircuit as ac\n@ac.encoding(width=3)\nclass State(Enum):\n    OFF = 0\n    ON = 3\n",
    "records.py": "import pycircuit as ac\n@ac.struct\nclass Parcel:\n    flag: ac.u1\n@ac.struct\nclass Spare:\n    flag: ac.u1\n",
    "empty.py": '"""A source unit with no declarations."""\n',
    "facade.py": f"from {PACKAGE}.enums import State as Mode\nfrom {PACKAGE}.records import Parcel as Data\n",
    "unrelated.py": 'from pycircuit import module\n@module\ndef Unrelated(flag: bool) -> {"out": bool}:\n    return {"out": flag}\n',
    "main.py": f'from {PACKAGE}.facade import Mode, Data\nfrom pycircuit import module\n@module\ndef Top(flag: bool) -> {{"out": bool}}:\n    return {{"out": flag}}\n',
}
OWNERS = {"enums.py", "records.py", "empty.py", "facade.py", "main.py"}


def _tool_hashes():
    return {
        name: hashlib.sha256((public._native() / "bin" / name).read_bytes()).hexdigest()
        for name in (
            "pycircuit-opt",
            "pycircuit-source-unit",
            "pycircuit-link",
            "pycircuit-emit",
        )
    }


@pytest.fixture(autouse=True)
def _stable_tools(tmp_path):
    before = _tool_hashes()
    yield
    after = _tool_hashes()
    (tmp_path / "native-tool-identity.json").write_text(
        json.dumps({"before": before, "after": after}, indent=2)
    )
    assert before == after, "native candidate changed during independent test"


def _compile(
    source, units, name, imports=(), *, output=None, replace=False, expected=0
):
    command = [
        "compile",
        "-c",
        str(source / name),
        "--source-root",
        str(source),
        "--package-prefix",
        PACKAGE,
        "-o",
        str(output or units / Path(name).stem),
    ]
    for unit in imports:
        command += ["-I", str(unit)]
    if replace:
        command.append("--replace")
    return public._cli(*command, expected=expected)


def _link(units, final, *, replace=False, expected=0):
    command = [
        "link",
        *(str(units / Path(name).stem) for name in sorted(OWNERS)),
        "--top",
        PACKAGE + ".main.Top",
        "-o",
        str(final),
    ]
    if replace:
        command.append("--replace")
    return public._cli(*command, expected=expected)


def _prepare(tmp_path):
    source, units = tmp_path / "source", tmp_path / "units"
    source.mkdir()
    units.mkdir()
    for name, text in SOURCES.items():
        (source / name).write_text(text, encoding="utf-8")
    for name in ("enums.py", "records.py", "empty.py", "unrelated.py"):
        _compile(source, units, name)
    _compile(source, units, "facade.py", (units / "enums", units / "records"))
    _compile(
        source,
        units,
        "main.py",
        (units / "enums", units / "records", units / "facade", units / "unrelated"),
    )
    final = tmp_path / "final.ac"
    _link(units, final)
    return source, units, final


def _expected_nominal_origins(filename):
    # Independent source AST positions and names, rather than emitter output.
    expected = []
    for index, node in enumerate(ast.parse(SOURCES[filename]).body):
        if not isinstance(node, ast.ClassDef):
            continue
        operation = "ac.enum" if node.bases else "ac.struct"
        symbol = f"{PACKAGE}.{Path(filename).stem}.{node.name}"
        origin = (
            '{expansion = [], site = {ast_path = [{kind = "field", name = "body"}, '
            f'{{kind = "index", value = {index} : i64}}], definition = @{symbol}}}}}'
        )
        expected.append(
            {
                "operation": operation,
                "origin": origin,
                "location": f'loc("{filename}":{node.lineno}:{node.col_offset + 1})',
            }
        )
    return expected


def _map_rows(payload):
    rows = {}
    for row in payload["source_maps"]:
        value = json.loads(row["text"])
        assert value["source"] == row["source"]
        owner = value["source"]["path"]
        assert owner not in rows
        rows[owner] = value
    return rows


def _generic(path, output):
    import subprocess

    command = [
        str(public._native() / "bin/pycircuit-opt"),
        str(path),
        "--mlir-print-op-generic",
        "--mlir-print-debuginfo",
        "-o",
        str(output),
    ]
    result = subprocess.run(
        command,
        env=public._environment(),
        capture_output=True,
        text=True,
        check=False,
        timeout=30,
    )
    assert result.returncode == 0, (command, result.stderr)
    return output.read_text()


def _value_span(text, key):
    match = re.search(r"(?<![\w.])" + re.escape(key) + r"\s*=\s*", text)
    assert match, key
    start = match.end()
    if text[start] not in "[{":
        found = re.match(r'"(?:\\.|[^"\\])*"|[^,}>\s]+', text[start:])
        assert found
        return start, start + found.end()
    depth = 0
    quoted = False
    escaped = False
    for i in range(start, len(text)):
        char = text[i]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char in "[{":
            depth += 1
        elif char in "]}":
            depth -= 1
            if depth == 0:
                return start, i + 1
    raise AssertionError("unterminated attribute")


def _replace_attribute(text, key, value):
    start, end = _value_span(text, key)
    return text[:start] + value + text[end:]


def _nominal_line(text, symbol):
    lines = text.splitlines(keepends=True)
    matches = [
        i
        for i, line in enumerate(lines)
        if f'sym_name = "{symbol}"' in line
        and ('"ac.enum"' in line or '"ac.struct"' in line)
    ]
    assert len(matches) == 1, (symbol, matches)
    return lines, matches[0]


def _mutate_nominal(text, symbol, key, value):
    lines, i = _nominal_line(text, symbol)
    lines[i] = _replace_attribute(lines[i], key, value)
    return "".join(lines)


def _remove_spare_export(text):
    start, end = _value_span(text, "ac.exports")
    value = text[start:end]
    pattern = r'\{name = "Spare", site = \{.*?\}, target = @enum_source_units\.records\.Spare\}'
    matches = list(re.finditer(pattern, value))
    assert len(matches) == 1
    a, b = matches[0].span()
    if value[b : b + 2] == ", ":
        b += 2
    elif value[a - 2 : a] == ", ":
        a -= 2
    return text[:start] + value[:a] + value[b:] + text[end:]


def _assert_failure(result, guard):
    assert result.stdout == ""
    assert "Traceback" not in result.stderr
    assert guard in result.stderr, result.stderr


def test_real_units_close_only_actual_provider_interfaces(tmp_path):
    source, units, final = _prepare(tmp_path)
    for name in ("enums", "records", "empty", "facade"):
        body = (units / name / f"{name}.ac").read_text()
        assert 'ac.unit_kind = "declarations"' in body
        assert '"ac.module"()' not in body
    top = (units / "main" / "main.interface.ac").read_text()
    assert '"unrelated.py"' not in top
    for name in ("enums.py", "records.py", "facade.py", "main.py"):
        assert f'"{name}"' in top
    depfile = (units / "main" / "main.d").read_text()
    assert str(units / "unrelated") not in depfile
    assert final.is_file() and source.is_dir()


def test_saved_final_groups_and_actual_origins_agree_across_targets(tmp_path):
    source, units, final = _prepare(tmp_path)
    shutil.rmtree(source)
    shutil.rmtree(units)
    cpp, rtl = maps._collect(final, "cpp"), maps._collect(final, "verilog")
    left, right = _map_rows(cpp), _map_rows(rtl)
    assert left.keys() == right.keys() == OWNERS
    for name in OWNERS:
        assert left[name]["source"] == right[name]["source"]
        assert left[name]["origins"] == right[name]["origins"]
    assert left["empty.py"]["origins"] == left["facade.py"]["origins"] == []
    for filename in ("enums.py", "records.py"):
        assert left[filename]["origins"] == _expected_nominal_origins(filename)
    for name in ("enums.py", "records.py", "empty.py", "facade.py"):
        group = next(g for g in cpp["source_groups"] if g["source"]["path"] == name)
        assert group["source_path"] is None and group["implementation"] is None
        assert group["header_path"] == f"sources/{PACKAGE}/{Path(name).stem}.hpp"
        assert left[name]["generated_files"] == [group["header_path"]]
        assert right[name]["generated_files"] == []
    assert {g["source"]["path"] for g in rtl["rtl_source_groups"]} == {"main.py"}
    assert all(g["path"] and g["path"].endswith(".v") for g in rtl["rtl_source_groups"])
    for target in ("cpp", "verilog"):
        output = tmp_path / target
        public._emit(final, target, output)
        receipt = public._receipt(output)
        assert {g["source"]["path"] for g in receipt["source_groups"]} == OWNERS
        roles = {f["path"]: f["role"] for f in receipt["files"]}
        for name in ("enums.py", "records.py", "empty.py", "facade.py"):
            group = next(
                g for g in receipt["source_groups"] if g["source"]["path"] == name
            )
            if target == "verilog":
                assert (
                    len(group["files"]) == 1
                    and roles[group["files"][0]] == "source-map"
                )
        assert not any(
            f["path"].endswith("/facade.v") or f["path"].endswith("/empty.v")
            for f in receipt["files"]
        )


def test_raw_final_metadata_failure_preserves_public_outputs(tmp_path):
    source, units, final = _prepare(tmp_path)
    canonical = _generic(final, tmp_path / "canonical.mlir")
    variants = {
        "inventory-type": (
            _replace_attribute(canonical, "ac.source_units", '"wrong"'),
            "ac.source_units must be an ArrayAttr",
        ),
        "owner-type": (
            _mutate_nominal(
                canonical, PACKAGE + ".enums.State", "ac.source_owner", '"wrong"'
            ),
            "valid SourceOwner dictionary",
        ),
        "origin-type": (
            _mutate_nominal(
                canonical, PACKAGE + ".enums.State", "ac.origin", '"wrong"'
            ),
            "final source map origin is not a valid Occurrence",
        ),
    }
    outputs = {}
    for target in ("cpp", "verilog"):
        output = tmp_path / (target + "-protected")
        public._emit(final, target, output)
        outputs[target] = (output, public._tree(output))
    for label, (text, guard) in variants.items():
        bad = tmp_path / (label + ".ac")
        bad.write_text(text)
        for target, (output, before) in outputs.items():
            fresh = tmp_path / (label + "-" + target)
            _assert_failure(public._emit(bad, target, fresh, expected=1), guard)
            assert not fresh.exists()
            _assert_failure(
                public._emit(bad, target, output, replace=True, expected=1), guard
            )
            assert public._tree(output) == before


def test_real_provider_mutations_preserve_fresh_and_replacement_finals(tmp_path):
    source, units, final = _prepare(tmp_path)
    original = final.read_bytes()
    bodies = {name: (units / name / f"{name}.ac") for name in ("enums", "records")}
    headers = {
        name: (units / name / f"{name}.interface.ac") for name in ("enums", "records")
    }
    texts = {
        p: _generic(p, tmp_path / (p.parent.name + "-" + p.name + ".generic"))
        for p in (*bodies.values(), *headers.values())
    }

    def run_bad(label, guard):
        fresh = tmp_path / (label + ".ac")
        _assert_failure(_link(units, fresh, expected=1), guard)
        assert not fresh.exists()
        _assert_failure(_link(units, final, replace=True, expected=1), guard)
        assert final.read_bytes() == original

    try:
        bodies["enums"].write_text(
            _mutate_nominal(
                texts[bodies["enums"]],
                PACKAGE + ".enums.State",
                "ac.source_owner",
                '{package = "enum_source_units", path = "facade.py"}',
            )
        )
        run_bad("foreign-owner", "SourceOwner")
        bodies["enums"].write_text(texts[bodies["enums"]])
        for label, path in (
            ("body-code", bodies["enums"]),
            ("header-code", headers["enums"]),
        ):
            path.write_text(
                texts[path].replace(
                    "code = #ac.math_int<3>", "code = #ac.math_int<4>", 1
                )
            )
            run_bad(label, "source body declaration differs from its owning header")
            path.write_text(texts[path])
        for path in (bodies["enums"], headers["enums"]):
            path.write_text(
                texts[path].replace(
                    "code = #ac.math_int<3>", "code = #ac.math_int<4>", 1
                )
            )
        run_bad("stale-consumers", "import snapshot differs")
        for path in (bodies["enums"], headers["enums"]):
            path.write_text(texts[path])
        body = _remove_spare_export(texts[bodies["records"]])
        lines, i = _nominal_line(body, PACKAGE + ".records.Spare")
        del lines[i]
        bodies["records"].write_text("".join(lines))
        headers["records"].write_text(_remove_spare_export(texts[headers["records"]]))
        run_bad(
            "missing-definition",
            "owning header nominal declaration has no matching body definition",
        )
    finally:
        for path, text in texts.items():
            path.write_text(text)
    _link(units, tmp_path / "restored.ac")


def test_compile_decl_failure_preserves_existing_unit(tmp_path):
    source, units, final = _prepare(tmp_path)
    before = public._tree(units / "enums")
    (source / "enums.py").write_text(SOURCES["enums.py"].replace("ON = 3", "ON = 8"))
    fresh = tmp_path / "fresh-unit"
    _assert_failure(
        _compile(source, units, "enums.py", output=fresh, expected=1), "width"
    )
    assert not fresh.exists()
    _assert_failure(
        _compile(source, units, "enums.py", replace=True, expected=1), "width"
    )
    assert public._tree(units / "enums") == before
    assert final.is_file()
