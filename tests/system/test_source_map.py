"""Final-only provenance on the current source route; no historical design claim."""

from __future__ import annotations

import ast
import json
import shutil
import subprocess
from pathlib import Path
from typing import Any

import pytest
import test_public_emit as public

pytestmark = pytest.mark.system
SOURCES = {
    "types.py": "import pycircuit as ac\n@ac.struct\nclass Word:\n    data: ac.u8\n@ac.struct\nclass UnusedWord:\n    data: ac.u4\n@ac.struct\nclass _PrivateWord:\n    data: ac.u3\n",
    "child.py": 'from pycircuit import module\n@module\ndef Child(value: bool) -> {"out": bool}:\n    return {"out": value}\n',
    "design_top.py": 'from source_public.child import Child\nfrom pycircuit import module, rule\n@module\ndef DesignTop(value: bool) -> {"left": bool, "right": bool}:\n    left = Child()\n    right = Child()\n    @rule\n    def bind_left():\n        left(value=value)\n    bind_left()\n    @rule\n    def bind_right():\n        right(value=value)\n    bind_right()\n    return {"left": left.out, "right": right.out}\n',
    "empty.py": '"""An empty provenance unit."""\n',
    "子.py": '"""An empty unit with a Unicode source path."""\n',
    "facade/__init__.py": "from source_public.types import Word as Word\n",
}


def _origin(symbol, components):
    path = ", ".join(
        (
            '{kind = "field", name = "' + value + '"}'
            if kind == "field"
            else f'{{kind = "index", value = {value} : i64}}'
        )
        for kind, value in components
    )
    return f"{{expansion = [], site = {{ast_path = [{path}], definition = @{symbol}}}}}"


def _expected_origins(filename):
    # Positions, names and paths come from source AST only, never emitted IR.
    expected = []
    for index, node in enumerate(ast.parse(SOURCES[filename]).body):
        if not isinstance(node, ast.FunctionDef | ast.ClassDef):
            continue
        symbol = f"source_public.{Path(filename).stem}.{node.name}"
        path = [("field", "body"), ("index", index)]
        operation = "ac.struct" if isinstance(node, ast.ClassDef) else "ac.module"
        expected.append(
            (
                operation,
                _origin(symbol, path),
                f'loc("{filename}":{node.lineno}:{node.col_offset + 1})',
            )
        )
        if isinstance(node, ast.FunctionDef):
            for ordinal, statement in enumerate(node.body):
                nested = path + [("field", "body"), ("index", ordinal)]
                if isinstance(statement, ast.Assign) and isinstance(
                    statement.value, ast.Call
                ):
                    site = statement
                    expected.append(
                        (
                            "ac.instance",
                            _origin(symbol, nested),
                            f'loc("{filename}":{site.lineno}:{site.col_offset + 1})',
                        )
                    )
                elif isinstance(statement, ast.FunctionDef):
                    expected.append(
                        (
                            "ac.rule",
                            _origin(symbol, nested),
                            f'loc("{filename}":{statement.lineno}:{statement.col_offset + 1})',
                        )
                    )
    return sorted(expected)


def _compile_current_closure(tmp_path, extended=False):
    source = tmp_path / "source"
    source.mkdir()
    names = ["types.py", "child.py", "design_top.py"]
    if extended:
        names += ["empty.py", "子.py", "facade/__init__.py"]
    units = tmp_path / "published" / "units"
    units.mkdir(parents=True)
    directories = {}
    for name in names:
        file = source / name
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(SOURCES[name], encoding="utf-8")
        directory = units / (
            "facade" if name.startswith("facade/") else Path(name).stem
        )
        command = [
            "compile",
            "-c",
            str(file),
            "--source-root",
            str(source),
            "--package-prefix",
            "source_public",
            "-o",
            str(directory),
        ]
        if name == "design_top.py":
            command += ["-I", str(directories["child.py"])]
        if name.startswith("facade/"):
            command += ["-I", str(directories["types.py"])]
        public._cli(*command)
        directories[name] = directory
    final = tmp_path / "published" / "final.ac"
    public._cli(
        "link",
        *(str(directories[name]) for name in names),
        "--top",
        "source_public.design_top.DesignTop",
        "-o",
        str(final),
    )
    shutil.rmtree(source)
    shutil.rmtree(units)
    return final


def _collect(final: Path, target: str) -> dict[str, Any]:
    command = [
        str(public._native() / "bin/pycircuit-emit"),
        str(final),
        "--target",
        target,
    ]
    result = subprocess.run(
        command,
        text=True,
        capture_output=True,
        check=False,
        timeout=180,
        env=public._environment(),
    )
    assert result.returncode == 0, f"{command!r}\n{result.stderr}"
    return json.loads(result.stdout)


def _map_rows(payload: dict[str, Any]) -> dict[str, dict[str, Any]]:
    rows = payload["source_maps"]
    result = {}
    expected_paths = {
        "facade/__init__.py": "sources/source_public/facade/__init__.source-map.json",
        "子.py": "sources/source_public/子.source-map.json",
    }
    for row in rows:
        value = json.loads(row["text"])
        source = value["source"]
        assert row["source"] == source
        expected_path = expected_paths.get(
            source["path"],
            f"sources/source_public/{Path(source['path']).stem}.source-map.json",
        )
        assert row["path"] == expected_path
        assert source["path"] not in result
        result[source["path"]] = value
    return result


def test_native_maps_inventory_exact_final_origins_for_both_targets(
    tmp_path: Path,
) -> None:
    final = _compile_current_closure(tmp_path)
    cpp_maps, rtl_maps = (
        _map_rows(_collect(final, "cpp")),
        _map_rows(_collect(final, "verilog")),
    )
    expected_sources = {"types.py", "child.py", "design_top.py"}
    assert cpp_maps.keys() == rtl_maps.keys() == expected_sources
    for filename in expected_sources:
        expected = _expected_origins(filename)
        for actual in (cpp_maps[filename], rtl_maps[filename]):
            assert [
                (r["operation"], r["origin"], r["location"]) for r in actual["origins"]
            ] == expected
        assert cpp_maps[filename]["source"] == rtl_maps[filename]["source"]
    placements = [
        r
        for r in cpp_maps["design_top.py"]["origins"]
        if r["operation"] == "ac.instance"
    ]
    assert len(placements) == 2
    assert sum(name == "child.py" for name in cpp_maps) == 1
    assert (
        len(
            [
                r
                for r in cpp_maps["child.py"]["origins"]
                if r["operation"] == "ac.module"
            ]
        )
        == 1
    )
    assert rtl_maps["types.py"]["generated_files"] == []
    assert cpp_maps["types.py"]["generated_files"] == [
        "sources/source_public/types.hpp"
    ]


def test_final_maps_cover_empty_facade_unicode_and_private_unused_units(
    tmp_path: Path,
) -> None:
    final = _compile_current_closure(tmp_path, extended=True)
    cpp_maps, rtl_maps = (
        _map_rows(_collect(final, "cpp")),
        _map_rows(_collect(final, "verilog")),
    )
    expected = {
        "child.py",
        "design_top.py",
        "types.py",
        "empty.py",
        "子.py",
        "facade/__init__.py",
    }
    assert cpp_maps.keys() == rtl_maps.keys() == expected
    for filename in ("empty.py", "子.py", "facade/__init__.py"):
        assert cpp_maps[filename]["origins"] == rtl_maps[filename]["origins"] == []
        assert rtl_maps[filename]["generated_files"] == []
    for filename in ("types.py", "empty.py", "子.py", "facade/__init__.py"):
        stem = (
            "facade/__init__" if filename.startswith("facade/") else Path(filename).stem
        )
        assert cpp_maps[filename]["generated_files"] == [
            f"sources/source_public/{stem}.hpp"
        ]
        assert rtl_maps[filename]["generated_files"] == []
    expected_types = _expected_origins("types.py")
    assert [
        (r["operation"], r["origin"], r["location"])
        for r in cpp_maps["types.py"]["origins"]
    ] == expected_types
    assert cpp_maps["types.py"]["origins"] == rtl_maps["types.py"]["origins"]


def test_removed_unreferenced_facade_inventory_is_not_historical_authentication(
    tmp_path: Path,
) -> None:
    # Real source link retains every supplied facade. A subsequently changed
    # native final carries only its retained provenance; it cannot authenticate
    # a deleted otherwise unreferenced source envelope.
    from test_enum_source_units import (
        OWNERS,
        PACKAGE,
        _generic,
        _prepare,
        _replace_attribute,
        _value_span,
    )

    _, _, final = _prepare(tmp_path)
    text = _generic(final, tmp_path / "generic.mlir")
    start, end = _value_span(text, "ac.source_units")
    inventory = text[start:end]
    row = '{package = "' + PACKAGE + '", path = "facade.py"}'
    assert inventory.count(row) == 1
    inventory = (
        inventory.replace(row + ", ", "", 1)
        if row + ", " in inventory
        else inventory.replace(", " + row, "", 1)
    )
    changed = tmp_path / "changed-native.ac"
    changed.write_text(
        _replace_attribute(text, "ac.source_units", inventory), encoding="utf-8"
    )
    for target in ("cpp", "verilog"):
        payload = _collect(changed, target)
        observed = {item["source"]["path"] for item in payload["source_maps"]}
        assert observed == OWNERS - {"facade.py"}
