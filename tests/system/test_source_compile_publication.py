"""End-to-end tests for the private per-source compile orchestration.

These use the real native source compiler: one source snapshot, explicitly
supplied managed interface units, and atomic publication of the four-file source
unit, then the existing link/emit path consumes the published artifacts and both
backends actually run.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
from pathlib import Path

import pytest

from pycircuit._publication import _PublicationError
from pycircuit._source_compile import _compile_source_unit
from pycircuit._source_unit_files import _load_full_source_unit

from test_generic_assignment_roundtrip import (
    BENCH,
    COPY_CHILD,
    TYPES,
    _cpp_trace,
    _rtl_trace,
    _run_cpp,
    _run_verilog,
)

pytestmark = pytest.mark.system


def _tool(env_name: str, name: str) -> str:
    configured = os.environ.get(env_name)
    if configured:
        return configured
    found = shutil.which(name)
    if not found:
        raise AssertionError(f"set {env_name} or put {name} on PATH")
    return found


def _native() -> str:
    return _tool("ACIR_SOURCE_UNIT_HARNESS", "acir-source-unit-harness")


def _design_harness() -> str:
    return _tool("ACIR_DESIGN_HARNESS", "acir-design-harness")


DECLARATION = TYPES

LEAF = """\
from pycircuit import module, rule
from .types import Word

@module
def Counter():
    state: Word = 254

    @rule
    def tick():
        nonlocal state
        state = (state + 1) & 255

    tick()
"""

BROKEN = """\
from pycircuit import module, rule

@module
def Broken():
    @rule
    def tick():
        return 1

    tick()
"""


def _workspace(tmp_path: Path) -> tuple[Path, Path]:
    root = tmp_path / "src"
    root.mkdir()
    units = tmp_path / "units"
    units.mkdir()
    return root, units


def _write(root: Path, name: str, text: str) -> Path:
    target = root / name
    target.write_text(text, encoding="utf-8")
    return target


def _compile(root: Path, units: Path, name: str, *, interface_units=(), replace=False):
    return _compile_source_unit(
        root / name, source_root=root, package="demo",
        native_compiler=_native(), output=units / Path(name).stem,
        interface_units=interface_units, replace=replace,
    )


def test_declaration_unit_closes_the_four_file_set(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    _write(root, "types.py", DECLARATION)

    result = _compile(root, units, "types.py")

    assert sorted(entry.name for entry in (units / "types").iterdir()) == [
        "types.ac", "types.d", "types.interface.ac", "unit.json",
    ]
    loaded = _load_full_source_unit(units / "types", owner=result.owner)
    # a declaration unit is a real declarations unit: no fabricated module or
    # root wrapper is created for a type-only source
    assert 'ac.unit_kind = "declarations"' in loaded.body
    assert '"ac.system"' not in loaded.body
    assert '"ac.module"' not in loaded.body


def test_parent_compiles_from_a_header_only_provider_with_hidden_child(
    tmp_path: Path,
) -> None:
    root, units = _workspace(tmp_path)
    _write(root, "types.py", DECLARATION)
    _write(root, "counter.py", LEAF)
    published = _compile(root, units, "types.py")

    # Hide the provider's Python source, body, and depfile: only its managed
    # header remains, which is all a parent may consume.
    (root / "types.py").unlink()
    (units / "types" / published.body).unlink()
    (units / "types" / published.depfile).unlink()

    dependent = _compile(root, units, "counter.py", interface_units=[units / "types"])

    assert sorted(entry.name for entry in (units / "counter").iterdir()) == [
        "counter.ac", "counter.d", "counter.interface.ac", "unit.json",
    ]
    depfile = (units / "counter" / dependent.depfile).read_text(encoding="utf-8")
    assert str(units / "types" / published.interface) in depfile
    assert str(units / "types" / "unit.json") in depfile
    assert str(units / "types" / published.body) not in depfile


def test_depfile_targets_the_published_artifact_and_excludes_scratch(
    tmp_path: Path,
) -> None:
    root, units = _workspace(tmp_path)
    _write(root, "types.py", DECLARATION)
    _write(root, "counter.py", LEAF)
    _compile(root, units, "types.py")
    dependent = _compile(root, units, "counter.py", interface_units=[units / "types"])

    depfile = (units / "counter" / dependent.depfile).read_text(encoding="utf-8")
    target = str(units / "counter" / "counter.ac")
    assert depfile.startswith(f"{target}: ")
    assert str(root / "counter.py") in depfile
    assert _native() in depfile
    for scratch_marker in ("transport", "header_", "consumed.json", "tmp"):
        assert scratch_marker not in depfile, scratch_marker


def test_same_owner_replace_updates_all_four_files(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    _write(root, "types.py", DECLARATION)
    first = _compile(root, units, "types.py")
    before = {
        entry.name: entry.read_bytes() for entry in (units / "types").iterdir()
    }

    _write(root, "types.py", DECLARATION + "\nAlias = Word\n")
    second = _compile(root, units, "types.py", replace=True)

    assert second.owner == first.owner
    after = {entry.name: entry.read_bytes() for entry in (units / "types").iterdir()}
    assert after["types.interface.ac"] != before["types.interface.ac"]
    loaded = _load_full_source_unit(units / "types", owner=second.owner)
    assert "Alias" in loaded.interface


def test_missing_and_broken_inputs_fail_closed(tmp_path: Path) -> None:
    root, units = _workspace(tmp_path)
    _write(root, "counter.py", LEAF)
    (units / "absent").mkdir()

    with pytest.raises(_PublicationError):
        _compile(root, units, "counter.py", interface_units=[units / "absent"])
    assert not (units / "counter").exists()

    _write(root, "broken.py", BROKEN)
    with pytest.raises(_PublicationError, match="rejected the source"):
        _compile(root, units, "broken.py")
    assert not (units / "broken").exists()


def test_published_units_drive_the_existing_link_and_both_backends(
    tmp_path: Path,
) -> None:
    """The entry's real artifacts feed the existing link/emit path, and a small
    design runs on both backends with an independently stated expectation."""
    root, units = _workspace(tmp_path)
    expected = [254, 3, 3]
    _write(root, "types.py", DECLARATION)
    _write(root, "holder.py", COPY_CHILD)
    _write(
        root, "test_holder.py",
        BENCH.format(arguments="other, state", reset_value=expected[0],
                     committed_value=expected[1]),
    )

    _compile(root, units, "types.py")
    _compile(root, units, "holder.py", interface_units=[units / "types"])
    _compile(
        root, units, "test_holder.py",
        interface_units=[units / "types", units / "holder"],
    )

    design = tmp_path / "test_holder.ac"
    command = [_design_harness()]
    for stem in ("types", "holder", "test_holder"):
        command.extend((
            "--body", str(units / stem / f"{stem}.ac"),
            "--header", str(units / stem / f"{stem}.interface.ac"),
        ))
    command.extend((
        "--top", "demo.test_holder.TestHolder", "--target", "final",
        "--role", "testbench", "--output", str(design),
    ))
    linked = subprocess.run(command, text=True, capture_output=True, check=False)
    assert linked.returncode == 0, linked.stderr

    cpp = tmp_path / "design.cpp"
    verilog = tmp_path / "design.sv"
    for target, output in (("cpp", cpp), ("verilog", verilog)):
        emitted = subprocess.run(
            [_design_harness(), "--design", str(design), "--target", target,
             "--role", "testbench", "--output", str(output)],
            text=True, capture_output=True, check=False,
        )
        assert emitted.returncode == 0, emitted.stderr

    run_dir = tmp_path / "run"
    run_dir.mkdir()
    cpp_out = _run_cpp(run_dir, cpp)
    assert _cpp_trace(cpp_out, "A") == expected, cpp_out
    assert _cpp_trace(cpp_out, "B") == expected, cpp_out

    rtl_values = _rtl_trace(_run_verilog(run_dir, verilog))
    per_run = expected + [1]
    assert rtl_values[: len(per_run)] == per_run, rtl_values
    assert rtl_values[len(per_run): 2 * len(per_run)] == per_run, rtl_values
