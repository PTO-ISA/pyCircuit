"""Final-only, dual-target provenance inventory checks for the M5 profile."""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path
from typing import Any

import pytest
import test_m5_public_emit as public

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]

# These expected provenance tuples come from the three small checked-in Python
# sources. They are deliberately independent of either target emitter's output.
EXPECTED_ORIGINS: dict[str, list[tuple[str, str, str]]] = {
    "counter.py": [
        (
            "ac.module",
            "{expansion = [], site = {ast_path = [], definition = @m5_public.counter.Counter}}",
            'loc("counter.py":8:1)',
        ),
        (
            "ac.rule",
            '{expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}], definition = @m5_public.counter.Counter}}',
            'loc("counter.py":12:5)',
        ),
    ],
    "design_top.py": [
        (
            "ac.instance",
            '{expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "value"}], definition = @m5_public.design_top.DesignTop}}',
            'loc("design_top.py":15:12)',
        ),
        (
            "ac.instance",
            '{expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 5 : i64}, {kind = "field", name = "value"}], definition = @m5_public.design_top.DesignTop}}',
            'loc("design_top.py":16:13)',
        ),
        (
            "ac.module",
            "{expansion = [], site = {ast_path = [], definition = @m5_public.design_top.DesignTop}}",
            'loc("design_top.py":9:1)',
        ),
    ],
    "types.py": [
        (
            "ac.type_alias",
            "{expansion = [], site = {ast_path = [], definition = @m5_public.types.UnusedWord}}",
            'loc("types.py":4:1)',
        ),
        (
            "ac.type_alias",
            "{expansion = [], site = {ast_path = [], definition = @m5_public.types.Word}}",
            'loc("types.py":3:1)',
        ),
        (
            "ac.type_alias",
            "{expansion = [], site = {ast_path = [], definition = @m5_public.types._PrivateWord}}",
            'loc("types.py":5:1)',
        ),
    ],
}


def _compile_extended_closure(tmp_path: Path) -> Path:
    source = tmp_path / "source"
    shutil.copytree(public.FIXTURE / "src", source)
    workspace = tmp_path / "published"
    final = public._compile_link(source, workspace, "m5_public")
    units = workspace / "units"
    extras = (
        ("empty.py", "empty"),
        ("子.py", "unicode"),
        ("facade/__init__.py", "facade"),
    )
    for relative, unit_name in extras:
        public._cli(
            "compile",
            "-c",
            str(source / relative),
            "--source-root",
            str(source),
            "--package-prefix",
            "m5_public",
            "-o",
            str(units / unit_name),
        )
    extended_final = workspace / "extended-final.ac"
    public._cli(
        "link",
        *(
            str(units / name)
            for name in ("types", "counter", "design_top", "empty", "unicode", "facade")
        ),
        "--top",
        "m5_public.design_top.DesignTop",
        "-o",
        str(extended_final),
    )
    shutil.rmtree(source)
    shutil.rmtree(units)
    assert final.is_file()
    return extended_final


def _collect(final: Path, target: str) -> dict[str, Any]:
    command = [
        str(public._native() / "bin/acir-cpp-source-parts-harness"),
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
        "facade/__init__.py": "sources/m5_public/facade/init.source-map.json",
        "子.py": "sources/m5_public/u5b50.source-map.json",
    }
    for row in rows:
        value = json.loads(row["text"])
        source = value["source"]
        assert row["source"] == source
        expected_path = expected_paths.get(
            source["path"],
            f"sources/m5_public/{Path(source['path']).stem}.source-map.json",
        )
        assert row["path"] == expected_path
        assert source["path"] not in result
        result[source["path"]] = value
    return result


def test_native_maps_inventory_exact_final_origins_for_both_targets(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    shutil.copytree(public.FIXTURE / "src", source)
    final = public._compile_link(source, tmp_path / "published", "m5_public")

    # Prove collection needs only final IR, not Python, body, or interface files.
    shutil.rmtree(source)
    shutil.rmtree(final.parent / "units")
    cpp_payload = _collect(final, "cpp")
    rtl_payload = _collect(final, "verilog")
    cpp_maps, rtl_maps = _map_rows(cpp_payload), _map_rows(rtl_payload)

    expected_sources = EXPECTED_ORIGINS.keys()
    assert cpp_maps.keys() == expected_sources
    assert rtl_maps.keys() == expected_sources
    for source_path, expected in EXPECTED_ORIGINS.items():
        observed_cpp = [
            (row["operation"], row["origin"], row["location"])
            for row in cpp_maps[source_path]["origins"]
        ]
        observed_rtl = [
            (row["operation"], row["origin"], row["location"])
            for row in rtl_maps[source_path]["origins"]
        ]
        assert observed_cpp == expected
        assert observed_rtl == expected
        assert cpp_maps[source_path]["source"] == rtl_maps[source_path]["source"]

    # Repeated child instances are two placement origins in their source unit;
    # the shared child definition still has only one source-owned map.
    child_instances = [
        row
        for row in cpp_maps["design_top.py"]["origins"]
        if row["operation"] == "ac.instance"
    ]
    assert len(child_instances) == 2
    assert sum(source_path == "counter.py" for source_path in cpp_maps) == 1

    # The declaration owner gets a map even though the RTL backend emits no
    # hardware member for that source.
    assert rtl_maps["types.py"]["generated_files"] == []
    assert cpp_maps["types.py"]["generated_files"] == ["sources/m5_public/types.hpp"]


def test_final_maps_cover_empty_facade_unicode_and_private_unused_units(
    tmp_path: Path,
) -> None:
    final = _compile_extended_closure(tmp_path)
    cpp_maps = _map_rows(_collect(final, "cpp"))
    rtl_maps = _map_rows(_collect(final, "verilog"))

    expected = {
        "counter.py",
        "design_top.py",
        "types.py",
        "empty.py",
        "子.py",
        "facade/__init__.py",
    }
    assert cpp_maps.keys() == expected
    assert rtl_maps.keys() == expected
    assert cpp_maps["empty.py"]["origins"] == []
    assert cpp_maps["empty.py"]["generated_files"] == ["sources/m5_public/empty.hpp"]
    assert rtl_maps["empty.py"]["generated_files"] == []
    assert cpp_maps["facade/__init__.py"]["generated_files"] == [
        "sources/m5_public/facade/init.hpp"
    ]
    assert rtl_maps["facade/__init__.py"]["generated_files"] == []
    assert cpp_maps["子.py"]["generated_files"] == ["sources/m5_public/u5b50.hpp"]
    assert rtl_maps["子.py"]["generated_files"] == []
    assert {row["location"] for row in cpp_maps["types.py"]["origins"]} == {
        'loc("types.py":3:1)',
        'loc("types.py":4:1)',
        'loc("types.py":5:1)',
    }
    assert cpp_maps["types.py"]["origins"] == rtl_maps["types.py"]["origins"]
